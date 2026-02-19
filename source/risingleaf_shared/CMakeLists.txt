cmake_minimum_required(VERSION 3.30)

set(LIB_NAME risingleaf_shared)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

list(APPEND SHARED_SOURCES
        graphics/View.cpp
        graphics/View.h
        graphics/Frustum.h
        graphics/graphics_toplevel_defines.h
        graphics/ShaderInfo.cpp
        graphics/ShaderInfo.h
        graphics/graphics_layer.h
        graphics/graphics_layer.cpp

        system/DebugMessenger.cpp
        system/DebugMessenger.h
        system/EventBridge.cpp
        system/EventBridge.h
        system/Events.cpp
        system/Events.h
        system/File.cpp
        system/File.h
        system/Log.cpp
        system/Log.h
        system/Timer.cpp
        system/Timer.h

)

if (APPLE)
    list(APPEND SHARED_SOURCES
            graphics/metal/graphics_metal.cpp
            graphics/metal/graphics_metal.h
    )
else ()
    list(APPEND SHARED_SOURCES
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

add_library(${LIB_NAME} STATIC ${SHARED_SOURCES})

target_include_directories(${LIB_NAME} PUBLIC
        ../
        ./
)

target_include_directories(${LIB_NAME} PUBLIC ./)

if (ASTROLATIVE_BUILD_SYSTEM STREQUAL "apple")
    target_include_directories(${LIB_NAME} PRIVATE external/metal-cpp/)
    target_include_directories(${LIB_NAME} PRIVATE external/metal-cpp-extensions/)
elseif (ASTROLATIVE_BUILD_SYSTEM STREQUAL "wasm")
    target_compile_definitions(${LIB_NAME} PUBLIC ASL_BUILD_WASM)
else ()
    find_package(Vulkan REQUIRED)
endif ()


if (NOT ASTROLATIVE_BUILD_SYSTEM STREQUAL "wasm")
    include(FetchContent)
    FetchContent_Declare(
            freetype
            GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
            GIT_TAG VER-2-14-1
    )
    FetchContent_MakeAvailable(freetype)

    if (NOT ASTROLATIVE_BUILD_SYSTEM STREQUAL "apple")
        find_package(Vulkan REQUIRED)
    endif ()

    target_include_directories(${LIB_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(${LIB_NAME} PUBLIC freetype ${Vulkan_LIBRARY_DIRS})
endif ()

if (ASTROLATIVE_BUILD_SYSTEM STREQUAL "apple")
    target_link_libraries(${LIB_NAME} PRIVATE
            "-framework Foundation"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework MetalKit"
    )
elseif (ASTROLATIVE_BUILD_SYSTEM STREQUAL "wasm")
    target_include_directories(${LIB_NAME} PUBLIC "${EMSCRIPTEN_SYSROOT}/include/freetype2/")
else ()
    target_link_libraries(${LIB_NAME} PRIVATE Vulkan::Vulkan)
endif ()
