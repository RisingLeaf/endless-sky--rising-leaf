//
// This file is part of Astrolative.
//
// Copyright (c) 2025 by Torben Hans
//
// Astrolative is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later version.
//
//  Astrolative is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with Astrolative. If not, see <https://www.gnu.org/licenses/>.
//
#include "EventBridge.h"

#include "Events.h"

void EventAdd_Resize()
{
  Events::Event e{};
  e.action = Events::Action::RESIZE;
  Events::AddEvent(e);
}

void EventAdd_MouseButton(const int button, const bool press, const double x_pos, const double y_pos)
{
  Events::Event e{};
  e.action = Events::Action::MOUSE_BUTTON;
  e.press  = press;
  e.key    = button;
  e.x      = x_pos;
  e.y      = y_pos;

  Events::AddEvent(e);
}

void EventAdd_MouseMove(const double x_pos, const double y_pos)
{
  Events::Event e{};
  e.action = Events::Action::MOUSE_MOVE;
  e.x = x_pos;
  e.y = y_pos;
  Events::AddEvent(e);
}

void EventAdd_Key(const int key, const char *key_char, const bool press, const double x_pos, const double y_pos)
{
  Events::Event e{ };
  e.action   = Events::Action::KEY;
  e.press    = press;
  e.key      = key;
  e.key_char = key_char;
  e.x        = x_pos;
  e.y        = y_pos;

  Events::AddEvent(e);
}

void EventAdd_Quit()
{
  Events::Event e{ };
  e.action = Events::Action::QUIT;

  Events::AddEvent(e);
}

#ifdef __APPLE__
#include "graphics/metal/graphics_metal.h"
void EventAdd_NewDrawable()
{
  graphics_metal::MessageNewDrawable();
}
#endif


