cmake_minimum_required(VERSION 3.18)

set(LIB_NAME astrolative_shared)

list(APPEND SHARED_SOURCES
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
if (ASTROLATIVE_BUILD_SYSTEM STREQUAL "apple")
  list(APPEND GRAPHICS_SOURCES
    graphics/metal/graphics_metal.cpp
    graphics/metal/graphics_metal.h
  )
elseif (ASTROLATIVE_BUILD_SYSTEM STREQUAL "wasm")
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
add_library(${LIB_NAME} STATIC ${SHARED_SOURCES} ${GRAPHICS_SOURCES})
target_include_directories(${LIB_NAME} PUBLIC
  ../
  ./
  external/
)

# Add stb implementation
add_subdirectory(external/stb)

# Includes and CompTime Definitions
if (ASTROLATIVE_BUILD_SYSTEM STREQUAL "apple")
  target_include_directories(${LIB_NAME} PRIVATE external/metal-cpp/)
  target_include_directories(${LIB_NAME} PRIVATE external/metal-cpp-extensions/)
  target_compile_definitions(${LIB_NAME} PUBLIC ASL_BUILD_APPLE)
elseif (ASTROLATIVE_BUILD_SYSTEM STREQUAL "wasm")
  target_compile_definitions(${LIB_NAME} PUBLIC ASL_BUILD_WASM)
else ()
  find_package(Vulkan REQUIRED)
endif ()

# Include parts that are by default in wasm
if (NOT ASTROLATIVE_BUILD_SYSTEM STREQUAL "wasm")
  if (NOT ASTROLATIVE_BUILD_SYSTEM STREQUAL "apple")
    find_package(Vulkan REQUIRED)
  endif ()

  target_include_directories(${LIB_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(${LIB_NAME} PUBLIC ${Vulkan_LIBRARY_DIRS})
endif ()

# Graphics frameworks
if (ASTROLATIVE_BUILD_SYSTEM STREQUAL "apple")
  target_link_libraries(${LIB_NAME} PRIVATE
    "-framework Foundation"
    "-framework Metal"
    "-framework QuartzCore"
    "-framework MetalKit"
  )
elseif (ASTROLATIVE_BUILD_SYSTEM STREQUAL "wasm")
else ()
  target_link_libraries(${LIB_NAME} PRIVATE Vulkan::Vulkan SDL3::SDL3)
endif ()

find_package(TBB REQUIRED)

# Base Libs
target_link_libraries(${LIB_NAME} PRIVATE ${CMAKE_DL_LIBS} TBB::tbb stblib)

set_target_properties(${LIB_NAME} PROPERTIES
  CXX_STANDARD 26
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
target_compile_options(${LIB_NAME} PRIVATE -freflection)
