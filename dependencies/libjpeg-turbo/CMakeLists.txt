include(ExternalProject)

ExternalProject_Add(
        libjpeg_turbo_ext
        GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
        GIT_TAG 3.1.3
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/libjpeg_install
        -DENABLE_SHARED=OFF
        -DENABLE_STATIC=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        INSTALL_DIR ${CMAKE_BINARY_DIR}/libjpeg_install
)

add_library(libjpeg_turbo STATIC IMPORTED GLOBAL)

set_target_properties(libjpeg_turbo PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/libjpeg_install/lib/libjpeg.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/libjpeg_install/include"
)

add_dependencies(libjpeg_turbo libjpeg_turbo_ext)