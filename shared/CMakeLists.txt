cmake_minimum_required(VERSION 3.18)

set(ASL_LIB_NAME astrolative_shared)

set(ASL_CXX_OPTIONS
      CXX_STANDARD 26
      CXX_STANDARD_REQUIRED YES
      CXX_EXTENSIONS NO
      CMAKE_CXX_EXTENSIONS OFF
      CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(ASL_BUILD_SYSTEM "invalid")
if (ASL_USE_WASM)
  set(ASL_BUILD_SYSTEM "wasm")
  set(CMAKE_SYSTEM_NAME Emscripten)

  list(APPEND USE_FLAGS_LIST
          -s MODULARIZE=1
          -s EXPORT_ES6=1
          -s USE_GLFW=3
          -s ASYNCIFY=1
          -s USE_WEBGL2=1
          -s MIN_WEBGL_VERSION=2
          -s MAX_WEBGL_VERSION=2
          -s NO_EXIT_RUNTIME=1
          -s ENVIRONMENT='web'
          -s ERROR_ON_UNDEFINED_SYMBOLS=0
          -s USE_SDL=2
          -s USE_FREETYPE=1
          -s ALLOW_MEMORY_GROWTH=1
          -s INITIAL_MEMORY=64MB
          -s MAXIMUM_MEMORY=512MB
          -s STACK_SIZE=5MB
          --preload-file ./resources/shaders
          --preload-file ../resources/)
  list(JOIN USE_FLAGS_LIST " " USE_FLAGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
  set(CMAKE_EXECUTABLE_SUFFIX .html)
elseif (APPLE)
  enable_language(Swift)
  set(ASL_BUILD_SYSTEM "apple")
elseif (UNIX)
  set(ASL_BUILD_SYSTEM "unix")
elseif (WIN32)
  set(ASL_BUILD_SYSTEM "windows")
else ()
  message(FATAL_ERROR "No valid build system found")
endif ()


list(APPEND ASL_SHARED_SOURCES
  base/ASLList.h
  base/ASLListAllocator.h
  base/ASLTypes.h
  base/ASLUtility.h
  base/concepts.h

  math/graphics_math/exterior.h
  math/graphics_math/field.h
  math/graphics_math/gm.h
  math/graphics_math/mat_helpers.h
  math/graphics_math/matrix.h
  math/graphics_math/vec_helpers.h
  math/graphics_math/vector.h
  math/noise/perlin.cpp
  math/noise/perlin.h
  math/noise/poisson.cpp
  math/noise/poisson.h
  math/qm/qm.h

  system/File.cpp
  system/File.h
  system/Log.cpp
  system/Log.h
  system/Timer.cpp
  system/Timer.h
)

list(APPEND GRAPHICS_SOURCES
  graphics/bootstrap/graphics_default_states.h
  graphics/bootstrap/graphics_quick_actions.h

  graphics/graphics_layer.cpp
  graphics/graphics_layer.h
  graphics/graphics_toplevel_defines.h
  graphics/ShaderInfo.cpp
  graphics/ShaderInfo.h)

# Platform specific source files
if (ASL_BUILD_SYSTEM STREQUAL "apple")
  list(APPEND GRAPHICS_SOURCES
    graphics/metal/graphics_metal.cpp
    graphics/metal/graphics_metal.h
  )
elseif (ASL_BUILD_SYSTEM STREQUAL "wasm")
  list(APPEND GRAPHICS_SOURCES
    graphics/wgpu/graphics_wgpu.cpp
    graphics/wgpu/graphics_wgpu.h
  )
else ()
  list(APPEND GRAPHICS_SOURCES
    graphics/vulkan/VulkanBootstrap.cpp
    graphics/vulkan/VulkanBootstrap.h
    graphics/vulkan/VulkanTranslate.h
    graphics/vulkan/VulkanHelpers.h
    graphics/vulkan/VulkanHelpers.cpp

    graphics/vulkan/VulkanDeviceInstance.cpp
    graphics/vulkan/VulkanDeviceInstance.h
    graphics/vulkan/VulkanCommandPool.cpp
    graphics/vulkan/VulkanCommandPool.h
    graphics/vulkan/VulkanCommandBuffer.cpp
    graphics/vulkan/VulkanCommandBuffer.h
    graphics/vulkan/VulkanDescriptorPool.cpp
    graphics/vulkan/VulkanDescriptorPool.h
    graphics/vulkan/VulkanPipelineState.h
    graphics/vulkan/VulkanRenderPassInstance.cpp
    graphics/vulkan/VulkanRenderPassInstance.h
    graphics/vulkan/VulkanShaderInstance.cpp
    graphics/vulkan/VulkanShaderInstance.h
    graphics/vulkan/VulkanBufferInstance.cpp
    graphics/vulkan/VulkanBufferInstance.h
    graphics/vulkan/VulkanTexture.cpp
    graphics/vulkan/VulkanTexture.h
    graphics/vulkan/VulkanFrameBufferInstance.cpp
    graphics/vulkan/VulkanFrameBufferInstance.h
    graphics/vulkan/VulkanSwapChainInstance.cpp
    graphics/vulkan/VulkanSwapChainInstance.h

    graphics/vulkan/graphics_vulkan.cpp
    graphics/vulkan/graphics_vulkan.h
  )
endif ()

# Project Includes
add_library(${ASL_LIB_NAME} STATIC ${ASL_SHARED_SOURCES} ${GRAPHICS_SOURCES})
target_include_directories(${ASL_LIB_NAME} PUBLIC ./ external/)

# Add stb implementation
add_subdirectory(external/stb)

# Includes and CompTime Definitions
if (ASL_BUILD_SYSTEM STREQUAL "apple")
  target_include_directories(${ASL_LIB_NAME} PRIVATE external/metal-cpp/)
  target_include_directories(${ASL_LIB_NAME} PRIVATE external/metal-cpp-extensions/)
  target_compile_definitions(${ASL_LIB_NAME} PUBLIC ASL_BUILD_APPLE)
elseif (ASL_BUILD_SYSTEM STREQUAL "wasm")
  target_compile_definitions(${ASL_LIB_NAME} PUBLIC ASL_BUILD_WASM)
else ()
  find_package(Vulkan REQUIRED)
endif ()

# Include parts that are by default in wasm
if (NOT ASL_BUILD_SYSTEM STREQUAL "wasm")
  if (NOT ASL_BUILD_SYSTEM STREQUAL "apple")
    find_package(Vulkan REQUIRED)
  endif ()

  target_include_directories(${ASL_LIB_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(${ASL_LIB_NAME} PUBLIC ${Vulkan_LIBRARY_DIRS})
endif ()

# Graphics frameworks
if (ASL_BUILD_SYSTEM STREQUAL "apple")
  target_link_libraries(${ASL_LIB_NAME} PRIVATE
    "-framework Foundation"
    "-framework Metal"
    "-framework QuartzCore"
    "-framework MetalKit"
  )
elseif (ASL_BUILD_SYSTEM STREQUAL "wasm")
else ()
  target_link_libraries(${ASL_LIB_NAME} PRIVATE Vulkan::Vulkan SDL3::SDL3)
endif ()

find_package(TBB REQUIRED)

# Base Libs
target_link_libraries(${ASL_LIB_NAME} PRIVATE ${CMAKE_DL_LIBS} TBB::tbb stblib)

set_target_properties(${ASL_LIB_NAME} PROPERTIES ${ASL_CXX_OPTIONS})
target_compile_options(${ASL_LIB_NAME} PUBLIC -freflection)
