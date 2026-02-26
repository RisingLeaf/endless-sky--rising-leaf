/* Shader.h
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
#include <memory>

#include "../risingleaf_shared/graphics/ShaderInfo.h"
#include "../risingleaf_shared/graphics/graphics_toplevel_defines.h"


namespace File {
	struct ShaderString;
}

// Class representing a shader, i.e. a compiled GLSL program that the GPU uses
// in order to draw something. In modern GPL, everything is drawn with shaders.
// In general, rather than using this class directly, drawing code will use one
// of the classes representing a particular shader.
class Shader {
public:
	explicit Shader(const std::string_view name) noexcept : Name(name) {}

	ShaderInfo &GetInfo() { return Info; }
	const ShaderInfo &GetInfo() const { return Info; }

	void Create(const std::vector<File::ShaderString> &shader_code);
	void Bind() const;

	bool operator()() const { return ShaderInstance.get(); }

  void Clear();

private:
	const std::string_view Name;

	std::unique_ptr<GraphicsTypes::ShaderInstance> ShaderInstance;

	ShaderInfo Info;
};
