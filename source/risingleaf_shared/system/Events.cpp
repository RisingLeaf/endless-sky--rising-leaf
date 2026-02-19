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
#include "Events.h"

#include <mutex>


namespace
{
  std::mutex event_mutex;
  std::vector<Events::Event> event_list;
}


void Events::AddEvent(Event event)
{
  std::lock_guard guard(event_mutex);

  event_list.emplace_back(event);
}

const std::vector<Events::Event> &Events::GetCurrentEvents()
{
  std::lock_guard guard(event_mutex);

  static std::vector<Event> copy;
  copy.clear();
  event_list.swap(copy);

  return copy;
}
