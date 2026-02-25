project("miniz" VERSION 3.1.1 LANGUAGES C)

add_library(miniz STATIC
    include/miniz.c
    include/miniz.h
)

target_include_directories(miniz PUBLIC include)