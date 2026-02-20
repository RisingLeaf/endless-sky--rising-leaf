include(FetchContent)
FetchContent_Declare(
        flac
        GIT_REPOSITORY https://github.com/xiph/flac.git
        GIT_TAG 1.5.0
)
set(INSTALL_MANPAGES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(flac)