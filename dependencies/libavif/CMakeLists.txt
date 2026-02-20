include(FetchContent)
FetchContent_Declare(
        libavif
        GIT_REPOSITORY https://github.com/AOMediaCodec/libavif.git
        GIT_TAG v1.3.0
)

set(AVIF_LIBYUV LOCAL CACHE STRING "" FORCE)

set(AVIF_CODEC_AOM LOCAL CACHE STRING "" FORCE)
set(AVIF_LIBSHARPYUV LOCAL CACHE STRING "" FORCE)
set(AVIF_JPEG LOCAL CACHE STRING "" FORCE)
set(AVIF_ZLIBPNG LOCAL CACHE STRING "" FORCE)
set(AVIF_ZLIBPNG LOCAL CACHE STRING "" FORCE)

FetchContent_MakeAvailable(libavif)