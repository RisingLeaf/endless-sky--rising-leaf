/* mat2.h
Copyright (c) 2026 by Torben Hans

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef ENDLESS_SKY_MAT2_H
#define ENDLESS_SKY_MAT2_H

namespace glsl
{
  struct mat2
  {
    float col0[2] = {0., 0.};
#ifndef __APPLE__
    float pad0[2] = {0.1234, 0.1234}; //for debugging
#endif
    float col1[2] = {0., 0.};
#ifndef __APPLE__
    float pad1[2] = {0.1234, 0.1234};
#endif
  };
}

#endif // ENDLESS_SKY_MAT2_H
