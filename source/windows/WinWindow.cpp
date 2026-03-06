/* WinWindow.cpp
Copyright (c) 2025 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "WinWindow.h"

#include "../Preferences.h"
#include "SDL3/SDL_version.h"
#include "WinVersion.h"


#include <dwmapi.h>


void WinWindow::UpdateTitleBarTheme(SDL_Window *window)
{
  if(!WinVersion::SupportsDarkTheme()) return;
}


void WinWindow::UpdateWindowRounding(SDL_Window *window)
{
  if(!WinVersion::SupportsWindowRounding()) return;
}
