include(FetchContent)
FetchContent_Declare(
    libpng
    URL https://sourceforge.net/projects/libpng/files/libpng16/1.6.55/libpng-1.6.55.tar.xz/download
    URL_HASH MD5=bc950b5a06ec1028285e14a127f6fb6e
)
FetchContent_MakeAvailable(libpng)