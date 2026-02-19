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
#include "DebugMessenger.h"

#include <cstring>
#include <mutex>
#include <unordered_map>


namespace Messenger
{
  std::mutex MessageMutex;
  std::unordered_map<std::string, std::pair<void *, size_t>> Messages;
}


void Messenger::SubmitMessage(const std::string &id, const void *data, size_t size)
{
  std::lock_guard guard(MessageMutex);
  auto it = Messages[id];
  if(it.first)
    free(it.first);

  it.first = new unsigned char[size];
  memcpy(it.first, data, size);
}


std::pair<const void *, size_t> Messenger::RetreiveMessage(const std::string &id)
{
  return Messages[id];
}

