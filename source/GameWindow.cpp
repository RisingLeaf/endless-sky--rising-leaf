/* GameWindow.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "GameWindow.h"

#include "Logger.h"
#include "Screen.h"

#include <SDL3/SDL.h>

#include <pthread/pthread.h>
#include <sstream>
#include <string>

#include "graphics/graphics_layer.h"

#ifdef _WIN32
#include "windows/WinVersion.h"

#include <windows.h>

#include <SDL3/SDL_syswm.h>
#include <dwmapi.h>
#endif

using namespace std;

namespace
{
  // The minimal screen resolution requirements.
  constexpr int minWidth  = 1024;
  constexpr int minHeight = 768;

  SDL_Window *mainWindow = nullptr;
  int         width      = 0;
  int         height     = 0;
  int         drawWidth  = 0;
  int         drawHeight = 0;

  // Logs SDL errors and returns true if found
  bool checkSDLerror()
  {
    string message = SDL_GetError();
    if(!message.empty())
    {
      Logger::LogError("(SDL message: \"" + message + "\")");
      SDL_ClearError();
      return true;
    }

    return false;
  }

  std::unique_ptr<GraphicsTypes::GraphicsInstance> Instance;
} // namespace


SDL_Window *GameWindow::GetWindow() { return mainWindow; }

GraphicsTypes::GraphicsInstance *GameWindow::GetInstance() { return Instance.get(); }


string GameWindow::SDLVersions()
{
  constexpr int built  = SDL_VERSION;
  const int     linked = SDL_GetVersion();

  return "Compiled against SDL v" + to_string(built) + "\nUsing SDL v" + to_string(linked);
}


bool GameWindow::Init(bool headless)
{
#ifdef _WIN32
  // Tell Windows this process is high dpi aware and doesn't need to get scaled.
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#elif defined(__linux__)
  // Set the class name for the window on Linux. Used to set the application icon.
  // This sets it for both X11 and Wayland.
  setenv("SDL_VIDEO_X11_WMCLASS", "io.github.endless_sky.endless_sky", true);
#endif

  // When running the integration tests, don't create a window nor an OpenGL context.
  if(headless)
#if defined(__linux__) && !SDL_VERSION_ATLEAST(2, 0, 22)
    setenv("SDL_VIDEODRIVER", "dummy", true);
#else
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
#endif

  // This needs to be called before any other SDL commands.
  const bool sdl_init = SDL_Init(SDL_INIT_VIDEO);
  if(!sdl_init)
  {
    checkSDLerror();
    return false;
  }

  int            count = 0;
  SDL_DisplayID *ids   = SDL_GetDisplays(&count);
  if(!count)
  {
    ExitWithError("No displays found!");
    return false;
  }

  // Get details about the current display.
  const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(ids[0]);
  if(!mode)
  {
    ExitWithError("Unable to query monitor resolution!");
    return false;
  }
  if(mode->refresh_rate && mode->refresh_rate < 60)
    Logger::LogError("Warning: low monitor frame rate detected (" + to_string(mode->refresh_rate) +
                     ")."
                     " The game will run more slowly.");

  SDL_free(ids);

  // Make the window just slightly smaller than the monitor resolution.
  const int maxWidth  = mode->w;
  const int maxHeight = mode->h;
  if(maxWidth < minWidth || maxHeight < minHeight)
    Logger::LogError("Monitor resolution is too small! Minimal requirement is " + to_string(minWidth) + 'x' + to_string(minHeight) + ", while your resolution is " + to_string(maxWidth) + 'x' +
                     to_string(maxHeight) + '.');

  int windowWidth  = maxWidth - 100;
  int windowHeight = maxHeight - 100;

  // Decide how big the window should be.
  if(Screen::RawWidth() && Screen::RawHeight())
  {
    // Load the previously saved window dimensions.
    windowWidth  = min(windowWidth, Screen::RawWidth());
    windowHeight = min(windowHeight, Screen::RawHeight());
  }

  // Settings that must be declared before the window creation.
#ifndef __APPLE__
  Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
  Uint32 flags = SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif


  if(Preferences::ScreenModeSetting() == "fullscreen") flags |= SDL_WINDOW_FULLSCREEN;
  else if(Preferences::Has("maximized")) flags |= SDL_WINDOW_MAXIMIZED;

  // The main window spawns visibly at this point.
  mainWindow = SDL_CreateWindow("Endless Sky", windowWidth, windowHeight, headless ? 0 : flags);

  if(!mainWindow)
  {
    ExitWithError("Unable to create window!");
    return false;
  }

  // Bail out early if we are in headless mode; no need to initialize all the OpenGL stuff.
  if(headless)
  {
    width  = windowWidth;
    height = windowHeight;
    Screen::SetRaw(width, height, true);
    return true;
  }

  // Make sure the screen size and view-port are set correctly.
  AdjustViewport(true);

#ifdef _WIN32
  UpdateTitleBarTheme();
  UpdateWindowRounding();
#endif

  Instance = graphics_layer::Init(width, height);

  return true;
}


// Clean up the SDL context, window, and shut down SDL.
void GameWindow::Quit()
{
  // Make sure the cursor is visible.
  SDL_ShowCursor();

  if(mainWindow) SDL_DestroyWindow(mainWindow);

  SDL_Quit();
}


void GameWindow::Step() { }


void GameWindow::AdjustViewport(bool noResizeEvent)
{
  if(!mainWindow) return;

  // Get the window's size in screen coordinates.
  int windowWidth, windowHeight;
  SDL_GetWindowSize(mainWindow, &windowWidth, &windowHeight);

  // Only save the window size when not in fullscreen mode.
  if(!GameWindow::IsFullscreen())
  {
    width  = windowWidth;
    height = windowHeight;
  }

  // Round the window size up to a multiple of 2, even if this
  // means one pixel of the display will be clipped.
  int roundWidth  = (windowWidth + 1) & ~1;
  int roundHeight = (windowHeight + 1) & ~1;
  Screen::SetRaw(roundWidth, roundHeight, noResizeEvent);

  // Find out the drawable dimensions. If this is a high- DPI display, this
  // may be larger than the window.
  SDL_GetWindowSizeInPixels(mainWindow, &drawWidth, &drawHeight);
  Screen::SetHighDPI(drawWidth > windowWidth || drawHeight > windowHeight);

  // Set the viewport to go off the edge of the window, if necessary, to get
  // everything pixel-aligned.
  drawWidth  = (drawWidth * roundWidth) / windowWidth;
  drawHeight = (drawHeight * roundHeight) / windowHeight;
}


// Attempts to set the requested SDL Window VSync to the given state. Returns false
// if the operation could not be completed successfully.
bool GameWindow::SetVSync(Preferences::VSync state) { return true; }


// Last window width, in windowed mode.
int GameWindow::Width() { return width; }


// Last window height, in windowed mode.
int GameWindow::Height() { return height; }


int GameWindow::DrawWidth() { return drawWidth; }


int GameWindow::DrawHeight() { return drawHeight; }


bool GameWindow::IsMaximized() { return SDL_GetWindowFlags(mainWindow) & SDL_WINDOW_MAXIMIZED; }


bool GameWindow::IsFullscreen() { return SDL_GetWindowFlags(mainWindow) & SDL_WINDOW_FULLSCREEN; }


void GameWindow::ToggleFullscreen()
{
  // This will generate a window size change event,
  // no need to adjust the viewport here.
  if(IsFullscreen())
  {
    SDL_SetWindowFullscreen(mainWindow, 0);
    SDL_SetWindowSize(mainWindow, width, height);
  }
  else SDL_SetWindowFullscreen(mainWindow, true);
}


void GameWindow::ExitWithError(const string &message, bool doPopUp)
{
  // Print the error message in the terminal and the error file.
  Logger::LogError(message);
  checkSDLerror();

  // Show the error message in a message box.
  if(doPopUp)
  {
    SDL_MessageBoxData box;
    box.flags       = SDL_MESSAGEBOX_ERROR;
    box.window      = nullptr;
    box.title       = "Endless Sky: Error";
    box.message     = message.c_str();
    box.colorScheme = nullptr;

    SDL_MessageBoxButtonData button;
    button.flags   = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    button.text    = "OK";
    box.numbuttons = 1;
    box.buttons    = &button;

    int result = 0;
    SDL_ShowMessageBox(&box, &result);
  }

  GameWindow::Quit();
}


#ifdef _WIN32
void GameWindow::UpdateTitleBarTheme()
{
  if(!WinVersion::SupportsDarkTheme()) return;

  SDL_SysWMinfo windowInfo;
  SDL_VERSION(&windowInfo.version);
  SDL_GetWindowWMInfo(mainWindow, &windowInfo);

  BOOL                       value;
  Preferences::TitleBarTheme themePreference = Preferences::GetTitleBarTheme();
  // If the default option is selected, check the system-wide preference.
  if(themePreference == Preferences::TitleBarTheme::DEFAULT)
  {
    HKEY systemPreference;
    if(RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &systemPreference) == ERROR_SUCCESS)
    {
      DWORD size = sizeof(value);
      if(RegQueryValueExW(systemPreference, L"AppsUseLightTheme", 0, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS)
        // The key says about light theme, while DWM expects information about dark theme.
        value = !value;
      else value = 1;
      RegCloseKey(systemPreference);
    }
    else value = 1;
  }
  else value = themePreference == Preferences::TitleBarTheme::DARK;

  HMODULE dwmapi                = LoadLibraryW(L"dwmapi.dll");
  auto    dwmSetWindowAttribute = reinterpret_cast<HRESULT (*)(HWND, DWORD, LPCVOID, DWORD)>(GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
  dwmSetWindowAttribute(windowInfo.info.win.window, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
  FreeLibrary(dwmapi);
}


void GameWindow::UpdateWindowRounding()
{
  if(!WinVersion::SupportsWindowRounding()) return;

  SDL_SysWMinfo windowInfo;
  SDL_VERSION(&windowInfo.version);
  SDL_GetWindowWMInfo(mainWindow, &windowInfo);

  auto value = static_cast<DWM_WINDOW_CORNER_PREFERENCE>(Preferences::GetWindowRounding());

  HMODULE dwmapi                = LoadLibraryW(L"dwmapi.dll");
  auto    dwmSetWindowAttribute = reinterpret_cast<HRESULT (*)(HWND, DWORD, LPCVOID, DWORD)>(GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
  dwmSetWindowAttribute(windowInfo.info.win.window, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value));
  FreeLibrary(dwmapi);
}
#endif
