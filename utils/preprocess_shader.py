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

glslc_executable   = sys.argv[1]
common_data_path   = sys.argv[2]
include_path       = sys.argv[3]
original_shader    = sys.argv[4]
destination_shader_vs = sys.argv[5] + ".prs.vert"
destination_shader_fs = sys.argv[5] + ".prs.frag"
destination_shader_cs = sys.argv[5] + ".prs.comp"

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

has_vert_shader = new_shader_code.find("VS_BEGIN") != -1
has_frag_shader = new_shader_code.find("FS_BEGIN") != -1
has_comp_shader = new_shader_code.find("CS_BEGIN") != -1

ubo_vars     = []
v_in_vars    = []
v_out_vars   = []
c_in_objects = []
textures     = []

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
        return "sampler2D"
    elif in_type == "2darray":
        return "sampler2DArray"
    elif in_type == "3d":
        return "sampler3D"
    else:
        print(f"ERROR: unknown texture type: {in_type}, defaulting to 2D")
        return "sampler2D"

def get_compute_texture_type(in_type):
    if   in_type == "2d":
        return "image2D"
    elif in_type == "2darray":
        return "image2DArray"
    elif in_type == "3d":
        return "image3D"
    else:
        print(f"ERROR: unknown texture type: {in_type}, defaulting to 2D")
        return "image2D"

pruned_code = ""

for line in new_shader_code.split("\n"):
    if  (  resolve_argument_line(ubo_vars,         "u_in",       line)
            or resolve_argument_line(v_in_vars,    "v_in",       line)
            or resolve_argument_line(v_out_vars,   "v_out",      line)
            or resolve_argument_line(c_in_objects, "cs_in",      line)
            or resolve_texture_line( textures,     "in_texture", line) ):
        continue
    pruned_code += line + "\n"

spec_struct = ""
if len(ubo_vars) > 0:
    spec_struct = "layout(set = 1, binding = 0, std140) uniform SpecUBO {\n"
    for var in ubo_vars:
        spec_struct += f"  {var[0]} {var[1]};\n"
    spec_struct += "} spec;\n"

v_in_num_elem = 0
v_in_struct = ""
for var in v_in_vars:
    v_in_struct += f"layout(location={v_in_num_elem}) in {var[0]} {var[1]};\n"
    v_in_num_elem += 1

v_out_num_elem = 0
v_out_struct = ""
for var in v_out_vars:
    v_out_struct += f"layout(location={v_out_num_elem}) out {var[0]} {var[1]};\n"
    v_out_num_elem += 1

f_in_num_elem = 0
f_in_struct = ""
for var in v_out_vars:
    f_in_struct += f"layout(location={f_in_num_elem}) in {var[0]} {var[1]};\n"
    f_in_num_elem += 1

textures_input_layout = ""
texture_counter       = 0
for tex in textures:
    textures_input_layout += f"layout(set = 2, binding = {texture_counter}) uniform {get_texture_type(tex[0])} {tex[1]};\n"
    texture_counter += 1

##### CREATE VS_BEGIN replacement
vs_begin_replacement = ""
vs_begin_replacement += v_in_struct
vs_begin_replacement += v_out_struct
vs_begin_replacement += "void main()\n" \
                        "{\n"

vs_header             = textures_input_layout

##### CREATE VS_END replacement
vs_end_replacement = "  gl_Position.x = -gl_Position.x; gl_Position.y = -gl_Position.y;\n}\n"


##### CREATE FS_BEGIN replacement
fs_begin_replacement  = ""
fs_begin_replacement += f_in_struct
fs_begin_replacement += f"layout(location = 0) out vec4 out_color;\n"
fs_begin_replacement += "void main()\n"
fs_begin_replacement += "{\n"

fs_header            = textures_input_layout

##### CREATE FS_END replacement
fs_end_replacement = "}"

##### CREATE CS_BEGIN replacement
cs_begin_replacement  = "void main()\n"
cs_begin_replacement += "{\n"

cs_header             = "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
if len(c_in_objects) > 0:
    tex_id = 0
    for c_in_object in c_in_objects:
        cs_header += f"layout(rgba8, set = 0, binding = {tex_id}) uniform {get_compute_texture_type(c_in_object[0])} {c_in_object[1]};\n"
        tex_id += 1

##### CREATE CS_END replacement
cs_end_replacement = "}"

header  = f"#version 430 core\n"
header += f"#define USE_TEXTURES\n"
header += f"#define PASS_TEXTURES\n"
header += f"#define USE_UBO\n"
header += f"#define PASS_UBO\n"
header += f"#define constant\n"
header += f"#define fmod(x, y) mod(x, y)\n"
header += f"#define M_PI 3.1415926535897932384626433832795\n"
header += "\n" + common_data_code

texture_counter = 0
header += spec_struct

header += "\n"

new_shader_code = pruned_code + "\n"

vs_begin_location = new_shader_code.find("VS_BEGIN")
vs_end_location   = new_shader_code.find("VS_END")

fs_begin_location = new_shader_code.find("FS_BEGIN")
fs_end_location   = new_shader_code.find("FS_END")

cs_begin_location = new_shader_code.find("CS_BEGIN")
cs_end_location   = new_shader_code.find("CS_END")

if has_vert_shader:
    vs_shader_code = new_shader_code
    if has_frag_shader:
        vs_shader_code = vs_shader_code[:fs_begin_location] + vs_shader_code[fs_end_location + len("FS_END"):]
    if has_comp_shader:
        vs_shader_code = vs_shader_code[:vs_shader_code.find("CS_BEGIN")] + vs_shader_code[vs_shader_code.find("CS_END") + len("CS_END"):]
    vs_shader_code = vs_shader_code.replace("VS_BEGIN", vs_begin_replacement)
    vs_shader_code = vs_shader_code.replace("VS_END", vs_end_replacement)
    vs_shader_code = header + vs_header + vs_shader_code
    if os.path.exists(destination_shader_vs):
        with open(destination_shader_vs, 'w') as file:
            file.write(vs_shader_code)
    else:
        with open(destination_shader_vs, 'x') as file:
            file.write(vs_shader_code)

if has_frag_shader:
    fs_shader_code = new_shader_code
    if has_vert_shader:
        fs_shader_code = fs_shader_code[:vs_begin_location] + fs_shader_code[vs_end_location + len("VS_END"):]
    if has_comp_shader:
        fs_shader_code = fs_shader_code[:fs_shader_code.find("CS_BEGIN")] + fs_shader_code[fs_shader_code.find("CS_END") + len("CS_END"):]
    fs_shader_code    = fs_shader_code.replace("FS_BEGIN", fs_begin_replacement)
    fs_shader_code    = fs_shader_code.replace("FS_END", fs_end_replacement)
    fs_shader_code = header + fs_header + fs_shader_code
    if os.path.exists(destination_shader_fs):
        with open(destination_shader_fs, 'w') as file:
            file.write(fs_shader_code)
    else:
        with open(destination_shader_fs, 'x') as file:
            file.write(fs_shader_code)

if has_comp_shader:
    cs_shader_code = new_shader_code
    if has_vert_shader:
        cs_shader_code = cs_shader_code[:vs_begin_location] + cs_shader_code[vs_end_location + len("VS_END"):]
    if has_frag_shader:
        cs_shader_code = cs_shader_code[:cs_shader_code.find("FS_BEGIN")] + cs_shader_code[cs_shader_code.find("FS_END") + len("FS_END"):]
    cs_shader_code    = cs_shader_code.replace("CS_BEGIN", cs_begin_replacement)
    cs_shader_code    = cs_shader_code.replace("CS_END",   cs_end_replacement)
    cs_shader_code = header + cs_header + cs_shader_code
    if os.path.exists(destination_shader_cs):
        with open(destination_shader_cs, 'w') as file:
            file.write(cs_shader_code)
    else:
        with open(destination_shader_cs, 'x') as file:
            file.write(cs_shader_code)

shader_pre_packaged = []
if has_vert_shader:
    os.system(f"{glslc_executable} -g {destination_shader_vs} -o {sys.argv[5]}.vert")
    shader_pre_packaged.append(('v', f"{sys.argv[5]}.vert"))
if has_frag_shader:
    os.system(f"{glslc_executable} -g {destination_shader_fs} -o {sys.argv[5]}.frag")
    shader_pre_packaged.append(('f', f"{sys.argv[5]}.frag"))
if has_comp_shader:
    os.system(f"{glslc_executable} -g {destination_shader_cs} -o {sys.argv[5]}.comp")
    shader_pre_packaged.append(('c', f"{sys.argv[5]}.comp"))

mode = ''
if os.path.exists(f"{sys.argv[5]}.ps"):
    mode = 'wb'
else:
    mode = 'xb'

with open(f"{sys.argv[5]}.ps", mode) as file:
    for literal, path in shader_pre_packaged:
        with open(path, 'rb') as shader_f:
            code = shader_f.read()
            length = len(code)

            file.write(literal.encode('ascii'))         # 1 byte
            file.write(struct.pack('<I', length))       # 4 bytes
            file.write(code)                            # length bytes







