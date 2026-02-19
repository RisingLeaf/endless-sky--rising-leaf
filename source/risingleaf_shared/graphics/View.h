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
#ifndef VIEW_H
#define VIEW_H

#include "math/graphics_math/gm.h"


class View {
  gm::dvec3 Position;
  gm::dvec3 Direction;
  gm::dvec3 Up;

public:
  View(const gm::dvec3 &p, const gm::dvec3 &u);

  void Update(double dx, double dy);

  [[nodiscard]] gm::dmat4 GetViewMatrix()           const { return gm::lookAt({0., 0., 0.}, Direction, Up); }
  [[nodiscard]] gm::dmat4 GetPositionalViewMatrix() const { return gm::lookAt(Position, Position + Direction, Up); }
  [[nodiscard]] const gm::dvec3 &GetPos()           const { return Position; }
  [[nodiscard]] const gm::dvec3 &GetDirection()     const { return Direction; }
  [[nodiscard]] const gm::dvec3 &GetUp()            const { return Up; }
  [[nodiscard]]       gm::dvec3  GetRight()         const { return gm::cross(Direction, Up); }
  void SetPos(const gm::dvec3 &pos) { Position = pos; }

  void SetUp(const gm::dvec3 &up) { Up = up; }
  void Move(const double dt, const int dir) { Position += 2. * Direction * dt * static_cast<double>(dir); }
};



#endif //VIEW_H
