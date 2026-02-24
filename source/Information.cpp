/* Information.cpp
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

#include "Information.h"

#include "image/Sprite.h"





void Information::SetRegion(const Rectangle &rect)
{
	region = rect;
	hasCustomRegion = true;
}



const Rectangle &Information::GetCustomRegion() const
{
	return region;
}



bool Information::HasCustomRegion() const
{
	return hasCustomRegion;
}



void Information::SetSprite(const std::string &name, const Sprite *sprite, const Point &unit,
	float frame, const Swizzle *swizzle)
{
	sprites[name] = sprite;
	spriteUnits[name] = unit;
	spriteFrames[name] = frame;
	spriteSwizzles[name] = swizzle;
}



const Sprite *Information::GetSprite(const std::string &name) const
{
	static const Sprite empty;

	auto it = sprites.find(name);
	return (it == sprites.end()) ? &empty : it->second;
}



const Point &Information::GetSpriteUnit(const std::string &name) const
{
	static const Point up(0., -1.);

	auto it = spriteUnits.find(name);
	return (it == spriteUnits.end()) ? up : it->second;
}



float Information::GetSpriteFrame(const std::string &name) const
{
	auto it = spriteFrames.find(name);
	return (it == spriteFrames.end()) ? 0.f : it->second;
}



const Swizzle *Information::GetSwizzle(const std::string &name) const
{
	auto it = spriteSwizzles.find(name);
	return it == spriteSwizzles.end() ? 0 : it->second;
}



void Information::SetString(const std::string &name, const std::string &value)
{
	strings[name] = value;
}



const std::string &Information::GetString(const std::string &name) const
{
	static const std::string empty;

	auto it = strings.find(name);
	return (it == strings.end()) ? empty : it->second;
}



void Information::SetBar(const std::string &name, double value, double segments)
{
	bars[name] = value;
	barSegments[name] = segments;
}



double Information::BarValue(const std::string &name) const
{
	auto it = bars.find(name);

	return (it == bars.end()) ? 0. : it->second;
}



double Information::BarSegments(const std::string &name) const
{
	auto it = barSegments.find(name);

	return (it == barSegments.end()) ? 1. : it->second;
}



void Information::SetCondition(const std::string &condition)
{
	conditions.insert(condition);
}



bool Information::HasCondition(const std::string &condition) const
{
	if(condition.empty())
		return true;

	if(condition.front() == '!')
		return !HasCondition(condition.substr(1));

	return conditions.contains(condition);
}



void Information::SetOutlineColor(const Color &color)
{
	outlineColor = color;
}



const Color &Information::GetOutlineColor() const
{
	return outlineColor;
}
