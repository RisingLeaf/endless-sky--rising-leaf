/* Point.cpp
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

#include "Point.h"

#include <algorithm>
#include <cmath>



Point::Point() noexcept
	: x(0.), y(0.)
{
}



Point::Point(const double x, const double y) noexcept
	: x(x), y(y)
{
}



// Check if the point is anything but (0, 0).
Point::operator bool() const noexcept
{
	return !this->operator!();
}



bool Point::operator!() const noexcept
{
	return x == 0. && y == 0.;
}



bool Point::operator==(const Point &other) const noexcept
{
	return x == other.x && y == other.y;
}



bool Point::operator!=(const Point &other) const noexcept
{
	return !(*this == other);
}



Point Point::operator+(const Point &point) const
{
	return {x + point.x, y + point.y};
}



Point &Point::operator+=(const Point &point)
{
	x += point.x;
	y += point.y;
	return *this;
}



Point Point::operator-(const Point &point) const
{
	return {x - point.x, y - point.y};
}



Point &Point::operator-=(const Point &point)
{
	x -= point.x;
	y -= point.y;
	return *this;
}



Point Point::operator-() const
{
	return Point() - *this;
}



Point Point::operator*(const double scalar) const
{
	return {x * scalar, y * scalar};
}



Point operator*(const double scalar, const Point &point)
{
	return {point.x * scalar, point.y * scalar};
}



Point &Point::operator*=(const double scalar)
{
	x *= scalar;
	y *= scalar;
	return *this;
}



Point Point::operator*(const Point &other) const
{
	return {x * other.x, y * other.y};
}



Point &Point::operator*=(const Point &other)
{
	x *= other.x;
	y *= other.y;
	return *this;
}



Point Point::operator/(const double scalar) const
{
	return {x / scalar, y / scalar};
}



Point &Point::operator/=(const double scalar)
{
	x /= scalar;
	y /= scalar;
	return *this;
}



void Point::Set(const double nx, const double ny)
{
	this->x = nx;
	this->y = ny;
}



// Operations that treat this point as a std::vector from (0, 0):
double Point::Dot(const Point &point) const
{
	return x * point.x + y * point.y;
}



double Point::Cross(const Point &point) const
{
	return x * point.y - y * point.x;
}



double Point::Length() const
{
	return std::sqrt(x * x + y * y);
}



double Point::LengthSquared() const
{
	return Dot(*this);
}



Point Point::Unit() const
{
	double b = x * x + y * y;
	if(b == 0.)
		return {1., 0.};
	b = 1. / sqrt(b);
	return {x * b, y * b};
}



double Point::Distance(const Point &point) const
{
	return (*this - point).Length();
}



double Point::DistanceSquared(const Point &point) const
{
	return (*this - point).LengthSquared();
}



Point Point::Lerp(const Point &to, const double c) const
{
	return *this + (to - *this) * c;
}



// Absolute value of both coordinates.
Point abs(const Point &p)
{
	return {std::abs(p.x), std::abs(p.y)};
}



// Take the min of the x and y coordinates.
Point min(const Point &p, const Point &q)
{
	return {std::min(p.x, q.x), std::min(p.y, q.y)};
}



// Take the max of the x and y coordinates.
Point max(const Point &p, const Point &q)
{
	return {std::max(p.x, q.x), std::max(p.y, q.y)};
}