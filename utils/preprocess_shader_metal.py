#!/usr/bin/env python3
#
# This file is part of Astrolative.
#
# Copyright (c) 2025 by Torben Hans
#
# Astrolative is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later version.
#
#  Astrolative is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
#  PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along with Astrolative. If not, see <https://www.gnu.org/licenses/>.
#
import os
import struct
import sys

common_data_path   = sys.argv[1]
include_path       = sys.argv[2]
original_shader    = sys.argv[3]
destination_shader = sys.argv[4] + ".prs"

with open(common_data_path, 'r') as file:
    common_data_code = file.read()

if len(common_data_code) < 1:
    print("Warning empty parse code given.")

with open(original_shader, 'r') as file:
    shader_code = file.read()

# Resolve include directives
keyword = "#include"
new_shader_code = ""
for line in shader_code.split("\n"):
    if line[0:len(keyword)] == keyword:
        args = line.split()
        if len(args) != 2:
            print("ERROR: wrong usage of '#include'")
        else:
            include_name = args[1]
            with open(include_path + include_name, 'r') as inc:
                new_shader_code += inc.read() + "\n"
    else:
        new_shader_code += line + "\n"

new_shader_code = new_shader_code.replace("//!COMMON_DATA", common_data_code)

ubo_vars      = []
v_in_vars     = []
v_out_vars    = []
c_in_objects  = []
textures      = []

def resolve_argument_line(to_list, identifier, in_line):
    pos = in_line.find(identifier)
    if pos > -1:
        pos_end = in_line.find(";")
        var_line = in_line[pos:pos_end]
        ids = var_line.split()
        if len(ids) != 3:
            print(f"ERROR: wrong usage of {identifier}: {in_line}")
        else:
            to_list.append((ids[1], ids[2]))
        return True
    return False

def resolve_texture_line(to_list, identifier, in_line):
    pos = in_line.find(identifier)
    if pos > -1:
        pos_end = in_line.find(";")
        var_line = in_line[pos:pos_end]
        ids = var_line.split()
        if len(ids) != 3:
            print(f"ERROR: wrong usage of {identifier}: {in_line}")
        else:
            to_list.append((ids[1], ids[2]))
        return True
    return False

def get_texture_type(in_type):
    if   in_type == "2d":
        return "texture2d<float>"
    elif in_type == "2darray":
        return "texture2d_array<float>"
    elif in_type == "3d":
        return "texture3d<float>"
    else:
        print(f"ERROR: unknown texture type: {in_type}, defaulting to 2d")
        return "texture2d<float>"

pruned_code = ""

for line in new_shader_code.split("\n"):
    if  (  resolve_argument_line(ubo_vars,      "u_in",       line)
        or resolve_argument_line(v_in_vars,     "v_in",       line)
        or resolve_argument_line(v_out_vars,    "v_out",      line)
        or resolve_argument_line(c_in_objects,  "cs_in",      line)
        or resolve_texture_line( textures,      "in_texture", line)):
        continue
    pruned_code += line + "\n"

spec_struct = "struct SpecUBO {\n"
for var in ubo_vars:
    spec_struct += f"  {var[0]} {var[1]};\n"
spec_struct += "};\n"

v_in_num_elem = 0
v_in_struct = "struct InVert {\n"
for var in v_in_vars:
    v_in_struct += f"  {var[0]} {var[1]}[[attribute({v_in_num_elem})]];\n"
    v_in_num_elem += 1
v_in_struct += "};\n"

v_out_struct = "struct OutVert {\n"
v_out_struct += "  float4 gl_Position[[position]];\n"
v_out_struct += "  float  gl_PointSize[[point_size]];\n"
for var in v_out_vars:
    v_out_struct += f"  {var[0]} {var[1]};\n"
v_out_struct += "};\n"

##### CREATE VS_BEGIN replacement
vs_begin_replacement = ""
vs_begin_replacement += spec_struct
vs_begin_replacement += v_in_struct
vs_begin_replacement += v_out_struct

vs_begin_replacement += "vertex OutVert vertexShader("\
                        "uint vertexID [[vertex_id]], " \
                        "InVert in_data [[stage_in]], "\
                        "constant const CmUBO   *in_glob [[buffer(2)]], "\
                        "constant const SpecUBO *in_spec [[buffer(3)]]"
texture_counter = 0
for tex in textures:
    vs_begin_replacement += f",\n    {get_texture_type(tex[0])} {tex[1]} [[texture({texture_counter})]]"
    texture_counter += 1

vs_begin_replacement += ")"\
                        "{\n"

vs_begin_replacement += "  OutVert out;\n"
for var in v_in_vars:
    vs_begin_replacement += f"  {var[0]} {var[1]} = in_data.{var[1]};\n"


vs_begin_replacement += f"  float4 gl_Position;\n"
for var in v_out_vars:
    vs_begin_replacement += f"  {var[0]} {var[1]};\n"

new_shader_code = pruned_code.replace("VS_BEGIN", vs_begin_replacement)

##### CREATE VS_END replacement
vs_end_replacement  = "  out.gl_Position = gl_Position;\n"
vs_end_replacement += "  out.gl_PointSize = 2.;\n"
for var in v_out_vars:
    vs_end_replacement += f"  out.{var[1]} = {var[1]};\n"
vs_end_replacement += "  return out;\n"
vs_end_replacement += "}\n"

new_shader_code = new_shader_code.replace("VS_END", vs_end_replacement)

##### CREATE FS_BEGIN replacement
fs_begin_replacement      = "fragment float4 fragmentShader(OutVert in [[stage_in]], " \
                            "constant const CmUBO   *in_glob [[buffer(2)]], " \
                            "constant const SpecUBO *in_spec [[buffer(3)]]"
texture_counter  = 0
f_texture_use_define  = ""
f_texture_pass_define = ""
for tex in textures:
    fs_begin_replacement  += f",\n    {get_texture_type(tex[0])} {tex[1]} [[texture({texture_counter})]]"
    f_texture_use_define  += f"{get_texture_type(tex[0])} {tex[1]} [[texture({texture_counter})]], "
    f_texture_pass_define += f"{tex[1]}, "
    texture_counter      += 1
fs_begin_replacement     += ")\n{\n"
fs_begin_replacement     += "  float4 out_color;\n"
for var in v_out_vars:
    fs_begin_replacement += f"  {var[0]} {var[1]} = in.{var[1]};\n"


new_shader_code    = new_shader_code.replace("FS_BEGIN", fs_begin_replacement)

fs_end_replacement = "  return out_color;\n}"

new_shader_code    = new_shader_code.replace("FS_END", fs_end_replacement)


## COMPUTE
cs_begin_replacement = "kernel void kernel_main(uint3 gl_GlobalInvocationID [[thread_position_in_grid]]"
if len(c_in_objects) > 0:
    tex_id = 0
    for c_in_object in c_in_objects:
        cs_begin_replacement += ",texture" + c_in_object[0] + "<float, access::write> " + c_in_object[1] + "[[texture(" + str(tex_id) + ")]]"
        tex_id += 1
cs_begin_replacement += "){\n"

new_shader_code = new_shader_code.replace("CS_BEGIN", cs_begin_replacement)
new_shader_code = new_shader_code.replace("CS_END", "}\n")

## Replace pointer access
new_shader_code    = new_shader_code.replace("glob.", "in_glob->")
new_shader_code    = new_shader_code.replace("spec.", "in_spec->")

header  = "#include <metal_stdlib>\n"
header += "using namespace metal;\n"
header += "#define discard discard_fragment()\n"
header += "\n"
header += "//Type conversions\n"
header += "typedef float2   vec2;\n"
header += "typedef float3   vec3;\n"
header += "typedef float4   vec4;\n"

header += "typedef int2     ivec2;\n"
header += "typedef int3     ivec3;\n"
header += "typedef int4     ivec4;\n"
header += "typedef float2x2 mat2;\n"
header += "typedef float3x3 mat3;\n"
header += "typedef float4x4 mat4;\n"
header += "\n"
header += "//Math functions\n"
header += "template<typename A, typename B> A atan(A x, B y) { return atan2(x, y); }\n"
header += "template<typename A> A fwidth(A x) { return abs(dfdx(x)) + abs(dfdy(x)); }\n"
header += "#define M_PI 3.1415926535897932384626433832795\n"
header += "\n"
header += "//Texture read\n"
header += "float4 texture(texture2d<float> tex, float2 point)\n"
header += "{\n"
header += "  constexpr sampler textureSampler (mag_filter::linear, min_filter::linear, mip_filter::linear, address::repeat);\n"
header += "  return tex.sample(textureSampler, float2(point.x, point.y));\n"
header += "}\n"
header += "float4 textureLod(texture2d<float> tex, float2 point, float lod)\n"
header += "{\n"
header += "  constexpr sampler textureSampler (mag_filter::linear, min_filter::linear, mip_filter::linear, address::repeat);\n"
header += "  return tex.sample(textureSampler, float2(point.x, point.y), level(lod));\n"
header += "}\n"
header += "float4 texture(texture2d_array<float> tex, float3 point)\n"
header += "{\n"
header += "  constexpr sampler textureSampler (mag_filter::linear, min_filter::linear, mip_filter::linear, address::repeat);\n"
header += "  return tex.sample(textureSampler, float2(point.x, point.y), point.z);\n"
header += "}\n"
header += "float4 texture(texture3d<float> tex, float3 point)\n"
header += "{\n"
header += "  constexpr sampler textureSampler (mag_filter::linear, min_filter::linear, mip_filter::linear, address::repeat);\n"
header += "  return tex.sample(textureSampler, point);\n"
header += "}\n"
header += "\n"
header += "//Texture write\n"
header += "void imageStore( texture3d<float, access::write> tex, int3 point, float4 value)\n"
header += "{\n"
header += "  tex.write(value, uint3(point), 0);\n"
header += "}\n\n"
header += "\n"
## Passing of objects that are only available in main scope in metal
header += f"#define USE_TEXTURES {f_texture_use_define}\n"
header += f"#define PASS_TEXTURES {f_texture_pass_define}\n"
header += f"#define USE_UBO constant const CmUBO *in_glob, constant const SpecUBO *in_spec,\n"
header += f"#define PASS_UBO in_glob, in_spec,\n"
header += f"typedef texture2d<float> sampler2D;\n"
header += f"typedef texture2d_array<float> sampler2DArray;\n"

## Final assembly
new_shader_code = header + new_shader_code + "\n"

mode = ''
if os.path.exists(f"{sys.argv[4]}.ps"):
    mode = 'wb'
else:
    mode = 'xb'

with open(f"{sys.argv[4]}.ps", mode) as file:
    length = len(new_shader_code.encode('ascii'))

    file.write('m'.encode('ascii'))                         # 1 byte
    file.write(struct.pack('<I', length))                   # 4 bytes
    file.write(new_shader_code.encode('ascii'))             # length bytes

## Write out
if os.path.exists(destination_shader):
    with open(destination_shader, 'w') as file:
        file.write(new_shader_code)
else:
    with open(destination_shader, 'x') as file:
        file.write(new_shader_code)