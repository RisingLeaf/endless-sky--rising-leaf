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
#ifndef EVENT_BRIDGE_H
#define EVENT_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif
  void EventAdd_Resize();
  void EventAdd_MouseButton(int button, bool press, double x_pos, double y_pos);
  void EventAdd_MouseMove(double x_pos, double y_pos);
  void EventAdd_Key(int key, const char *key_char, bool press, double x_pos, double y_pos);
  void EventAdd_Quit();
#ifdef __APPLE__
  void EventAdd_NewDrawable();
#endif

#ifdef __cplusplus
}
#endif

#endif //EVENT_BRIDGE_H
