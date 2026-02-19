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
#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <algorithm>
#include <array>
#include "math/graphics_math/gm.h"

struct Frustum
{
  struct Plane
  {
    gm::dvec3 Normal = gm::dvec3(0.);
    double    D      = 0.;

    Plane() = default;

    Plane(const gm::dvec3& norm, const double d)
      : Normal(norm), D(d) {}

    [[nodiscard]] double SignedDistance(const gm::dvec3& point) const
    {
      return gm::dot(Normal, point) + D;
    }
  };

  std::array<Plane, 6> Planes{};

  static Frustum CreateFrustum(const gm::dmat4& view, const gm::dmat4& projection)
  {
    Frustum frustum;

    const gm::dmat4 clip = projection * view;

    const gm::dvec4 row0 = {clip[0, 0], clip[1, 0], clip[2, 0], clip[3, 0]};
    const gm::dvec4 row1 = {clip[0, 1], clip[1, 1], clip[2, 1], clip[3, 1]};
    const gm::dvec4 row2 = {clip[0, 2], clip[1, 2], clip[2, 2], clip[3, 2]};
    const gm::dvec4 row3 = {clip[0, 3], clip[1, 3], clip[2, 3], clip[3, 3]};

    auto makePlane = [](const gm::dvec4& rowA, const gm::dvec4& rowB) -> Plane {
      const gm::dvec4 plane = rowA + rowB;
      gm::dvec3 normal{plane[0], plane[1], plane[2]};
      double d = plane[3];
      if (const double length = gm::length(normal); length > 0.0)
      {
        normal /= length;
        d /= length;
      }
      //else
      //  Log::Warn<<"Zero normal on frustum create!"<<Log::End;
      return { normal, d };
    };

    /*
    Log::Info<<"Mat : "<<std::to_string(clip)<<Log::End;
    Log::Info<<"Row0: "<<std::to_string(row0)<<Log::End;
    Log::Info<<"Row1: "<<std::to_string(row1)<<Log::End;
    Log::Info<<"Row2: "<<std::to_string(row2)<<Log::End;
    Log::Info<<"Row3: "<<std::to_string(row3)<<Log::End;
    */

    frustum.Planes[0] = makePlane(row3, row0);       // Left
    frustum.Planes[1] = makePlane(row3, -1. * row0); // Right
    frustum.Planes[2] = makePlane(row3, row1);       // Bottom
    frustum.Planes[3] = makePlane(row3, -1. * row1); // Top
    frustum.Planes[4] = makePlane(row3, row2);       // Near
    frustum.Planes[5] = makePlane(row3, -row2);      // Far

    return frustum;
  }

  [[nodiscard]] bool SphereInside(const gm::dvec3& pos, const double radius) const
  {
    return std::ranges::all_of(Planes, [&](const Plane& plane)
    {
      return plane.SignedDistance(pos) > -radius;
    });
  }
};

#endif // FRUSTUM_H
