/* Shader.cpp
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

#include "Shader.h"

#include <string>
#include <vector>

#include "../GameWindow.h"
#include "../risingleaf_shared/graphics/graphics_layer.h"
#include "../risingleaf_shared/system/File.h"
#include "../risingleaf_shared/system/Log.h"





void Shader::Create(const std::vector<File::ShaderString> &shader_code)
{
	Log::Info<<"Compiling Shader: "<<Name<<Log::End;

  GameWindow::GetInstance()->CreateShader(ShaderInstance, Info, shader_code, Name);

	if (!ShaderInstance)
		throw std::runtime_error("Shader Instance not found: " + std::string(Name));
}

void Shader::Bind() const
{
	GameWindow::GetInstance()->BindShader(ShaderInstance.get());
}


