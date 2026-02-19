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
#ifndef DEBUGMESSENGER_H
#define DEBUGMESSENGER_H

#include <string>

namespace Messenger
{
  void SubmitMessage(const std::string &id, const void *data, size_t size);
  std::pair<const void *, size_t> RetreiveMessage(const std::string &id);
}

#endif //DEBUGMESSENGER_H
