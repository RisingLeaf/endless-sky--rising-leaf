//
// This file is part of Astrolative.
//
// Copyright (c) 2026 by Torben Hans
//
// Astrolative is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later version.
//
//  Astrolative is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with Astrolative. If not, see
//  <https://www.gnu.org/licenses/>.
//

#ifndef ASTROLATIVE_POISSON_H
#define ASTROLATIVE_POISSON_H

#include <vector>

#include "shared/math/graphics_math/gm.h"

namespace poisson
{
  bool disc(std::vector<gm::vec2> &set, float R, const gm::vec2 &p, const gm::vec2 &a, const gm::vec2 &b);
  void disc(std::vector<gm::vec2> &set, size_t k, const gm::vec2 &a, const gm::vec2 &b, asl::uint64 seed);
} // namespace poisson


#endif // ASTROLATIVE_POISSON_H
