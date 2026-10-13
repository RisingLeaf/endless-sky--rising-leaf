include(FetchContent)
include(ExternalProject)

#
# SDL3
#
function(fetch_sdl3)
    set(SDL_X11_XTEST OFF CACHE STRING "" FORCE)
    FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG        release-3.4.18
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(SDL3)
endfunction()

#
# libpng
#
function(fetch_libpng)
    FetchContent_Declare(
            libpng
            GIT_REPOSITORY https://github.com/pnggroup/libpng.git
            GIT_TAG        v1.6.47
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(libpng)
endfunction()

#
# libjpeg-turbo
#
include(ExternalProject)

function(fetch_libjpeg_turbo)
    set(LIBJPEG_TURBO_INSTALL_DIR "${CMAKE_BINARY_DIR}/libjpeg-turbo-install")
    set(LIBJPEG_TURBO_STATIC_LIB  "${LIBJPEG_TURBO_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jpeg${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ExternalProject_Add(libjpeg_turbo
            GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
            GIT_TAG        3.1.0
            GIT_SHALLOW    TRUE

            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
                -DENABLE_SHARED=OFF
                -DENABLE_STATIC=ON
                -DWITH_TURBOJPEG=ON
                -DWITH_TEST=OFF
                -DWITH_TOOLS=OFF
            BUILD_BYPRODUCTS
                "${LIBJPEG_TURBO_STATIC_LIB}"
    )
    add_library(JPEG::JPEG STATIC IMPORTED GLOBAL)
    set_target_properties(JPEG::JPEG PROPERTIES
            IMPORTED_LOCATION
            "${LIBJPEG_TURBO_STATIC_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES
            "${LIBJPEG_TURBO_INSTALL_DIR}/include"
    )
    add_dependencies(JPEG::JPEG libjpeg_turbo)
endfunction()


#
# libavif
#
function(fetch_libavif)
    set(AVIF_LIBYUV      LOCAL CACHE STRING "" FORCE)
    set(AVIF_CODEC_AOM   OFF   CACHE STRING "" FORCE)
    set(AVIF_CODEC_DAV1D OFF   CACHE STRING "" FORCE)
    set(AVIF_CODEC_RAV1E OFF   CACHE STRING "" FORCE)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_Declare(
            libavif
            GIT_REPOSITORY https://github.com/AOMediaCodec/libavif.git
            GIT_TAG v1.2.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(libavif)
endfunction()

#
# libopenal-soft
#
function(fetch_openal)
    FetchContent_Declare(
            openal
            GIT_REPOSITORY https://github.com/kcat/openal-soft.git
            GIT_TAG        1.24.3
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(openal)
endfunction()

#
# libmad
#
function(fetch_libmad)
    FetchContent_Declare(libmad URL ftp://ftp.mars.org/pub/mpeg/libmad-0.15.0b.tar.gz)
    FetchContent_MakeAvailable(libmad)
    add_library(mad STATIC
            ${libmad_SOURCE_DIR}/bit.c
            ${libmad_SOURCE_DIR}/decoder.c
            ${libmad_SOURCE_DIR}/fixed.c
            ${libmad_SOURCE_DIR}/frame.c
            ${libmad_SOURCE_DIR}/huffman.c
            ${libmad_SOURCE_DIR}/layer12.c
            ${libmad_SOURCE_DIR}/layer3.c
            ${libmad_SOURCE_DIR}/stream.c
            ${libmad_SOURCE_DIR}/synth.c
            ${libmad_SOURCE_DIR}/timer.c
            ${libmad_SOURCE_DIR}/version.c
    )

    target_include_directories(mad PUBLIC ${libmad_SOURCE_DIR})
    target_compile_definitions(mad PRIVATE FPM_DEFAULT)

    target_compile_options(mad PRIVATE -include stdlib.h)
endfunction()

#
# Flac++
#
function(fetch_flac)
    set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(INSTALL_MANPAGES OFF CACHE BOOL "" FORCE)
    set(BUILD_CXXLIBS ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
            flac
            GIT_REPOSITORY https://github.com/xiph/flac.git
            GIT_TAG        1.5.0
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(flac)
endfunction()

#
# zlib
#
function(fetch_zlib)
    FetchContent_Declare(
            zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG        v1.3.1
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(zlib)
endfunction()

#
# minizip(zlib)
#
function(fetch_minizip)
    FetchContent_Declare(
            zlib
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG        v1.3.1
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(zlib)

    add_library(minizip STATIC
            ${zlib_SOURCE_DIR}/contrib/minizip/ioapi.c
            ${zlib_SOURCE_DIR}/contrib/minizip/mztools.c
            ${zlib_SOURCE_DIR}/contrib/minizip/unzip.c
            ${zlib_SOURCE_DIR}/contrib/minizip/zip.c
    )
    target_include_directories(minizip
            PUBLIC
            ${zlib_SOURCE_DIR}
            ${zlib_SOURCE_DIR}/contrib/
    )
    target_link_libraries(minizip PUBLIC zlibstatic)
endfunction()

#
# enet
#
function(fetch_enet)
    FetchContent_Declare(
            enet
            GIT_REPOSITORY https://github.com/lsalzman/enet.git
            GIT_TAG        v1.3.18
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(enet)
    target_include_directories(enet PUBLIC "${enet_SOURCE_DIR}/include")
endfunction()