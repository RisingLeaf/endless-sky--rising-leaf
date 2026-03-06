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

#include <sstream>
#include <string>

#include "graphics/graphics_layer.h"
#include "windows/WinWindow.h"


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
    const std::string message = SDL_GetError();
    if(!message.empty())
    {
      Logger::Log( "(SDL message: \"" + message + "\")", Logger::Level::ERROR);
      SDL_ClearError();
      return true;
    }

    return false;
  }

  std::unique_ptr<GraphicsTypes::GraphicsInstance> Instance;
} // namespace


SDL_Window *GameWindow::GetWindow() { return mainWindow; }

GraphicsTypes::GraphicsInstance *GameWindow::GetInstance() { return Instance.get(); }


std::string GameWindow::SDLVersions()
{
  constexpr int built  = SDL_VERSION;
  const int     linked = SDL_GetVersion();

  return "Compiled against SDL v" + std::to_string(built) + "\nUsing SDL v" + std::to_string(linked);
}


bool GameWindow::Init(bool headless)
{
#ifdef _WIN32
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
  {
    Logger::Log(
        "low monitor frame rate detected (" + std::to_string(mode->refresh_rate) +
        ")."
        " The game will run more slowly.", Logger::Level::WARNING);
  }

  SDL_free(ids);

  // Make the window just slightly smaller than the monitor resolution.
  const int maxWidth  = mode->w;
  const int maxHeight = mode->h;
  if(maxWidth < minWidth || maxHeight < minHeight)
  {
    Logger::Log(
        "Monitor resolution is too small! Minimal requirement is " + std::to_string(minWidth) + 'x' +
        std::to_string(minHeight) + ", while your resolution is " + std::to_string(maxWidth) + 'x' +
        std::to_string(maxHeight) + '.', Logger::Level::ERROR);
  }

  int windowWidth  = maxWidth - 100;
  int windowHeight = maxHeight - 100;

  // Decide how big the window should be.
  if(Screen::RawWidth() && Screen::RawHeight())
  {
    // Load the previously saved window dimensions.
    windowWidth  = std::min(windowWidth, Screen::RawWidth());
    windowHeight = std::min(windowHeight, Screen::RawHeight());
  }

  if(!Preferences::Has("Block screen saver")) SDL_EnableScreenSaver();

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
  Instance.reset();

  // Make sure the cursor is visible.
  SDL_ShowCursor();

  if(mainWindow) SDL_DestroyWindow(mainWindow);

  SDL_Quit();
}


void GameWindow::Step() {}


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
  else {
    SDL_SetWindowFullscreen(mainWindow, true);
  }
}

void GameWindow::ToggleBlockScreenSaver()
{
  if(SDL_ScreenSaverEnabled()) SDL_DisableScreenSaver();
  else SDL_EnableScreenSaver();
}


void GameWindow::ExitWithError(const std::string &message, bool doPopUp)
{
  // Print the error message in the terminal and the error file.
  Logger::Log(message, Logger::Level::ERROR);
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
    button.flags    = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    button.text     = "OK";
    box.numbuttons  = 1;
    box.buttons     = &button;

    int result = 0;
    SDL_ShowMessageBox(&box, &result);
  }

  GameWindow::Quit();
}


#ifdef _WIN32
void GameWindow::UpdateTitleBarTheme() { WinWindow::UpdateTitleBarTheme(mainWindow); }


void GameWindow::UpdateWindowRounding() { WinWindow::UpdateWindowRounding(mainWindow); }
#endif
