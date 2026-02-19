//
// This file is part of Astrolative.
//
// Copyright (in[2, 0]) 2025 by Torben Hans
//
// Astrolative is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later version.
//
//  Astrolative is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR in[0, 0]
//  PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received in[0, 0] copy of the GNU General Public License along with Astrolative. If not, see <https://www.gnu.org/licenses/>.
//

#ifndef MAT_HELPERS_H
#define MAT_HELPERS_H

#include "matrix.h"
#include "base/ASLUtility.h"

#include <cmath>

namespace gm
{
  template<typename Type>
  constexpr matrix<Type, 2> inverse(const matrix<Type, 2> &in)
  {
    matrix<Type, 2> inv;

    const Type det = in[0, 0] * in[1, 1] - in[1, 0] * in[0, 1];

    if (asl::abs(det) <= std::numeric_limits<Type>::epsilon())
      return {};

    inv[0, 0] =  in[1, 1]; inv[0, 1] = -in[0, 1];
    inv[1, 0] = -in[1, 0]; inv[1, 1] =  in[0, 0];

    inv *= static_cast<Type>(1) / det;
    return inv;
  }

  template<typename Type>
  constexpr matrix<Type, 3> inverse(const matrix<Type, 3>& m)
  {
    matrix<Type, 3> inv;

    inv[0, 0] = m[1, 1]*m[2, 2] - m[1, 2]*m[2, 1]; inv[0, 1] = m[0, 2]*m[2, 1] - m[0, 1]*m[2, 2]; inv[0, 2] = m[0, 1]*m[1, 2] - m[0, 2]*m[1, 1];
    inv[1, 0] = m[1, 2]*m[2, 0] - m[1, 0]*m[2, 2]; inv[1, 1] = m[0, 0]*m[2, 2] - m[0, 2]*m[2, 0]; inv[1, 2] = m[0, 2]*m[1, 0] - m[0, 0]*m[1, 2];
    inv[2, 0] = m[1, 0]*m[2, 1] - m[1, 1]*m[2, 0]; inv[2, 1] = m[0, 1]*m[2, 0] - m[0, 0]*m[2, 1]; inv[2, 2] = m[0, 0]*m[1, 1] - m[0, 1]*m[1, 0];

    Type det =
        m[0, 0]*inv[0, 0] +
        m[0, 1]*inv[1, 0] +
        m[0, 2]*inv[2, 0];

    if (asl::abs(det) <= std::numeric_limits<Type>::epsilon())
      return {};

    inv *= static_cast<Type>(1) / det;
    return inv;
  }

  template<typename Type>
  constexpr Type det3(const matrix<Type, 4>& m, int skip_r, int skip_c)
  {
    matrix<Type, 3> sub;
    int r = 0;

    for (int i = 0; i < 4; ++i)
    {
      if (i == skip_r) continue;
      int c = 0;
      for (int j = 0; j < 4; ++j)
      {
        if (j == skip_c) continue;
        sub[r, c++] = m[i, j];
      }
      ++r;
    }
    return
        sub[0, 0]*(sub[1, 1]*sub[2, 2] - sub[2, 1]*sub[1, 2]) -
        sub[1, 0]*(sub[0, 1]*sub[2, 2] - sub[2, 1]*sub[0, 2]) +
        sub[2, 0]*(sub[0, 1]*sub[1, 2] - sub[1, 1]*sub[0, 2]);
  }

  template<typename Type>
  constexpr matrix<Type, 4> inverse(const matrix<Type, 4>& m)
  {
    matrix<Type, 4> inv;

    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        const Type minor = det3(m, j, i);
        const Type sign  = (i + j) & 1 ? Type(-1) : Type(1);
        inv[i, j] = sign * minor;
      }
    }

    Type det =
        m[0, 0]*inv[0, 0] +
        m[1, 0]*inv[0, 1] +
        m[2, 0]*inv[0, 2] +
        m[3, 0]*inv[0, 3];

    if (asl::abs(det) <= std::numeric_limits<Type>::epsilon())
      return {};

    inv *= static_cast<Type>(1) / det;
    return inv;
  }

  template<typename Type>
  constexpr matrix<Type, 3> orthonormalize(const matrix<Type, 3> &in)
  {
    using vec_type = vector<Type, 3>;
    using mat_type = matrix<Type, 3>;
    mat_type r     = in;

    vec_type r0 = {r[0, 0], r[0, 1], r[0, 2]};
    vec_type r1 = {r[1, 0], r[1, 1], r[1, 2]};
    vec_type r2 = {r[2, 0], r[2, 1], r[2, 2]};
    r0          = normalize(r0);

    Type d0 = dot(r0, r1);
    r1 -= r0 * d0;
    r1 = normalize(r1);

    const Type d1 = dot(r1, r2);
    d0            = dot(r0, r2);
    r2 -= r0 * d0 + r1 * d1;
    r2  = normalize(r2);

    r = {
      r0[0], r1[0], r2[0],
      r0[1], r1[1], r2[1],
      r0[2], r1[2], r2[2],
    };

    return r;
  }

  template<typename Type>
  constexpr matrix<Type, 4> rotate(const matrix<Type, 4> &base, const Type angle, const vector<Type, 3> &axis)
  {
    const Type cos_theta     = std::cos(angle);
    const Type sub_cos_theta = static_cast<Type>(1) - cos_theta;
    const Type sin_theta     = std::sin(angle);
    matrix<Type, 4> rotator = {
              cos_theta + axis[0]*axis[0]*sub_cos_theta, axis[0]*axis[1]*sub_cos_theta - axis[2]*sin_theta, axis[0]*axis[2]*sub_cos_theta + axis[1]*sin_theta, 0,
      axis[0]*axis[1]*sub_cos_theta + axis[2]*sin_theta,         cos_theta + axis[1]*axis[1]*sub_cos_theta, axis[1]*axis[2]*sub_cos_theta - axis[0]*sin_theta, 0,
      axis[0]*axis[2]*sub_cos_theta - axis[1]*sin_theta, axis[1]*axis[2]*sub_cos_theta + axis[0]*sin_theta,         cos_theta + axis[2]*axis[2]*sub_cos_theta, 0,
                                                      0,                                                 0,                                                 0, 1
    };

    return rotator * base;
  }

  template<typename Type>
  constexpr matrix<Type, 4> translate(const matrix<Type, 4> &base, const vector<Type, 3> &move)
  {
    matrix<Type, 4> translator = {
      1, 0, 0, move[0],
      0, 1, 0, move[1],
      0, 0, 1, move[2],
      0, 0, 0,       1,
    };

    return translator * base;
  }

  template<typename Type>
  constexpr matrix<Type, 4> scale(const matrix<Type, 4> &base, const vector<Type, 3> &scale)
  {
    matrix<Type, 4> scaler = {
      scale[0],        0,        0, 0,
             0, scale[1],        0, 0,
             0,        0, scale[2], 0,
             0,        0,        0, 1,
    };

    return scaler * base;
  }

  template<typename Type>
  constexpr matrix<Type, 4> lookAt(vector<Type, 3> const& eye, vector<Type, 3> const& center, vector<Type, 3> const& up)
  {
    vector<Type, 3> const f(normalize(center - eye));
    vector<Type, 3> const s(normalize(cross(up, f)));
    vector<Type, 3> const u(cross(f, s));

    matrix<Type, 4> result(1);
    result[0, 0] = s[0];
    result[1, 0] = s[1];
    result[2, 0] = s[2];
    result[0, 1] = u[0];
    result[1, 1] = u[1];
    result[2, 1] = u[2];
    result[0, 2] = f[0];
    result[1, 2] = f[1];
    result[2, 2] = f[2];
    result[3, 0] = -dot(s, eye);
    result[3, 1] = -dot(u, eye);
    result[3, 2] = -dot(f, eye);

    return result;
  }

  template<typename Type>
  constexpr matrix<Type, 4> perspective(const Type fov, const Type aspect, const Type z_near, const Type z_far)
  {
    const Type tan_half_fov = std::tan(fov / static_cast<Type>(2));

    matrix<Type, 4> result;
    result[0, 0] = static_cast<Type>(1) / (aspect * tan_half_fov);
    result[1, 1] = static_cast<Type>(1) / (tan_half_fov);
    result[2, 2] = z_far / (z_far - z_near);
    result[2, 3] = static_cast<Type>(1);
    result[3, 2] = -(z_far * z_near) / (z_far - z_near);
    return result;
  }
}

#endif //MAT_HELPERS_H
