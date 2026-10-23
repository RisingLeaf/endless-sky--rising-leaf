/* Angle.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Point.h"

#include <cstdint>


// Represents an angle, in degrees. Angles are in "clock" orientation rather
// than usual mathematical orientation. That is, 0 degrees is up, and angles
// increase in a clockwise direction. Angles can be efficiently mapped to unit
// vectors, which also makes rotating a std::vector an efficient operation.
class Angle {
public:
	// Return a random angle up to the given amount (between 0 and 360).
	static Angle Random();
	static Angle Random(double range);


public:
	// The default constructor creates an angle pointing up (zero degrees).
	Angle() noexcept = default;
	// Construct an Angle from the given angle in degrees. Allow this conversion
	// to be implicit to allow syntax like "angle += 30".
	Angle(double degrees) noexcept;
	// Construct an angle pointing in the direction of the given std::vector.
	explicit Angle(const Point &point) noexcept;

	// Mathematical operators.
	Angle operator+(const Angle &other) const;
	Angle &operator+=(const Angle &other);
	Angle operator-(const Angle &other) const;
	Angle &operator-=(const Angle &other);
	Angle operator-() const;

	bool operator==(const Angle &other) const;
	bool operator!=(const Angle &other) const;

	// Get a unit std::vector in the direction of this angle.
	[[nodiscard]] Point Unit() const;
	// Convert an Angle object to degrees, in the range -180 to 180.
	[[nodiscard]] double Degrees() const;
	// Convert an Angle object to degrees, in the range 0 to 360.
	[[nodiscard]] double AbsDegrees() const;

	// Return a point rotated by this angle around (0, 0).
	[[nodiscard]] Point Rotate(const Point &point) const;

	// Judge whether this is inside from "base" to "limit."
	// The range from "base" to "limit" is expressed by "clock" orientation.
	[[nodiscard]] bool IsInRange(const Angle &base, const Angle &limit) const;

	[[nodiscard]] int32_t Raw() const { return angle; }

	static Angle FromRaw(const int32_t raw) { return Angle(raw); }

private:
	explicit Angle(int32_t angle);


private:
	// The angle is stored as an integer value between 0 and 2^16 - 1. This is
	// so that any angle can be mapped to a unit std::vector (a very common operation)
	// with just a single array lookup. It also means that "wrapping" angles
	// to the range of 0 to 360 degrees can be done via a bit mask.
	int32_t angle = 0;
};
