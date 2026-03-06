/* Dictionary.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Dictionary.h"

#include "StringInterner.h"

#include <cstring>
#include <string>
#include <algorithm>


namespace {
	std::pair<size_t, bool> Search(const char *key, const std::vector<std::pair<const char *, double>> &v)
	{
    const auto it = std::lower_bound(
      v.begin(), v.end(), key,
      [](const auto &pair, const char *k) { return strcmp(pair.first, k) < 0; }
    );

	  return {static_cast<size_t>(it - v.begin()), it != v.end() && strcmp(it->first, key) == 0};
	}
}



double &Dictionary::operator[](const char *key)
{
	auto [loc, found] = Search(key, *this);
	if(found)
		return data()[loc].second;

	return insert(begin() + loc, std::make_pair(StringInterner::Intern(key), 0.))->second;
}



double &Dictionary::operator[](const std::string &key)
{
	return (*this)[key.c_str()];
}



double Dictionary::Get(const char *key) const
{
	auto [loc, found] = Search(key, *this);
	return found ? data()[loc].second : 0.;
}



double Dictionary::Get(const std::string &key) const
{
	return Get(key.c_str());
}



void Dictionary::Erase(const char *key)
{
	auto [pos, exists] = Search(key, *this);
	if(exists)
		erase(next(this->begin(), pos));
}