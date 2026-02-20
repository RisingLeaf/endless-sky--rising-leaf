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
        ./
        ../
)


if (APPLE)
    target_include_directories(${LIB_NAME} PRIVATE ./external/metal-cpp/)
    target_include_directories(${LIB_NAME} PRIVATE ./external/metal-cpp-extensions/)
else ()
    find_package(Vulkan REQUIRED)
endif ()


include(FetchContent)
FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
        GIT_TAG VER-2-14-1
)
FetchContent_MakeAvailable(freetype)

if (NOT APPLE)
    find_package(Vulkan REQUIRED)
endif ()

target_include_directories(${LIB_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${LIB_NAME} PUBLIC freetype ${Vulkan_LIBRARY_DIRS} SDL3::SDL3)

if (APPLE)
    target_link_libraries(${LIB_NAME} PRIVATE
            "-framework Foundation"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework MetalKit"
    )
else ()
    target_link_libraries(${LIB_NAME} PRIVATE Vulkan::Vulkan)
endif ()
