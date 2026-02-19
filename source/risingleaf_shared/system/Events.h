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
#ifndef EVENTS_HPP
#define EVENTS_HPP

#include <vector>
#include <string>

namespace Events
{
  enum class Action
  {
    NONE,
    RESIZE,
    MOUSE_BUTTON,
    MOUSE_MOVE,
    KEY,
    QUIT,
  };

  struct Event
  {
    Action           action   = Action::NONE;
    bool             press    = false;
    std::string      key_char;
    int              key      = -1;
    double           x        = 0.;
    double           y        = 0.;
  };

  void AddEvent(Event event);
  const std::vector<Event> &GetCurrentEvents();
}

#endif //EVENTS_HPP
