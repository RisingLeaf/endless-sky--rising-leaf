cmake_minimum_required(VERSION 3.18)


file(GLOB GLSL_SOURCE_FILES "*.pps")

list(LENGTH GLSL_SOURCE_FILES shader_count)
list(JOIN   GLSL_SOURCE_FILES "\n   " shader_out_string)
message(STATUS "Found ${shader_count} shaders:\n   ${shader_out_string}")

if(ASTROLATIVE_BUILD_SYSTEM STREQUAL "apple")
    foreach(GLSL ${GLSL_SOURCE_FILES})
        get_filename_component(GLSL_NAME ${GLSL} NAME_WLE)
        get_filename_component(GLSL_EXTENSION ${GLSL} EXT)
        set(PRE_PROCESSED ${CMAKE_SOURCE_DIR}/shaders/compilation/${GLSL_NAME}.prs${GLSL_EXTENSION})
        set(SPIRV ${CMAKE_SOURCE_DIR}/shaders/compilation/${GLSL_NAME})
        add_custom_command(
                OUTPUT ${SPIRV}.ps
                COMMAND
                ${CMAKE_SOURCE_DIR}/tools/preprocess_shader_metal.py ${CMAKE_SOURCE_DIR}/shaders/common_uniform_data.metal ${CMAKE_SOURCE_DIR}/shaders/ ${GLSL} ${SPIRV}
                DEPENDS ${GLSL} ${CMAKE_SOURCE_DIR}/tools/preprocess_shader_metal.py)
        list(APPEND PRE_PROCESSED_FILES ${SPIRV}.ps)
    endforeach(GLSL)

    add_custom_target(Shaders DEPENDS ${PRE_PROCESSED_FILES})
else ()
    message("SPIRV Compiler: ${Vulkan_GLSLC_EXECUTABLE}")

    foreach(GLSL ${GLSL_SOURCE_FILES})
        get_filename_component(GLSL_NAME ${GLSL} NAME_WLE)
        set(PRE_PROCESSED ${CMAKE_SOURCE_DIR}/shaders/compilation/${GLSL_NAME})
        set(SPIRV ${CMAKE_SOURCE_DIR}/shaders/compilation/${GLSL_NAME})
        add_custom_command(
                OUTPUT ${SPIRV}.vert.spv ${SPIRV}.frag.spv
                COMMAND
                ${CMAKE_SOURCE_DIR}/tools/preprocess_shader.py ${Vulkan_GLSLC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/shaders/common_uniform_data.glsl ${CMAKE_SOURCE_DIR}/shaders/ ${GLSL} ${PRE_PROCESSED}
                DEPENDS ${GLSL} ${CMAKE_SOURCE_DIR}/tools/preprocess_shader.py)
        list(APPEND SPIRV_BINARY_FILES ${SPIRV}.vert.spv)
        list(APPEND SPIRV_BINARY_FILES ${SPIRV}.frag.spv)
    endforeach(GLSL)

    add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})
endif ()
