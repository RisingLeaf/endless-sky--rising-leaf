include(FetchContent)
FetchContent_Declare(
        openal-soft
        GIT_REPOSITORY https://github.com/kcat/openal-soft.git
        GIT_TAG 1.25.1
)
FetchContent_MakeAvailable(openal-soft)