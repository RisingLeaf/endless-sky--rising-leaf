cmake_minimum_required(VERSION 3.20)
project(STBLIB CXX)


add_library(stblib STATIC
  stb_image.h
  stb_write_image.h
  stb_implementation.cpp
)

if (MSVC)
  target_compile_options(stblib PRIVATE /w)
else()
  target_compile_options(stblib PRIVATE -w)
endif()