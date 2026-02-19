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
#include "View.h"


View::View(const gm::dvec3 &p, const gm::dvec3 &u)
: Position(p), Up(u)
{
  Direction = gm::normalize(-1. * Position);
}



void View::Update(const double dx, const double dy)
{
  Direction = gm::dvec3(gm::normalize(gm::rotate(gm::rotate(gm::dmat4(1.), dx, Up), dy, gm::cross(Up, Direction)) * gm::dvec4(Direction, 0.)));
}

