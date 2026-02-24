include(FetchContent)
FetchContent_Declare(
        ogg
        GIT_REPOSITORY https://github.com/xiph/ogg.git
        GIT_TAG v1.3.6
)
FetchContent_MakeAvailable(ogg)
FetchContent_Declare(
        flac
        GIT_REPOSITORY https://github.com/xiph/flac.git
        GIT_TAG 1.5.0
)
set(INSTALL_MANPAGES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(flac)