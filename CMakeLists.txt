cmake_minimum_required(VERSION 3.25)

project(libtpumat
    VERSION   1.0.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(libtpumat_SOURCES
    tpu.cpp
)

set(libtpumat_HEADERS
    libtpumat/tpu.hpp
    libtpumat/export.hpp
)

add_library(libtpumat SHARED
    ${libtpumat_SOURCES}
    ${libtpumat_HEADERS}
)

target_link_libraries(libtpumat PUBLIC
    libtpumem
)

target_include_directories(libtpumat PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_compile_definitions(libtpumat PRIVATE
        LIBTPUMAT_BUILDING_DLL
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
endif()

if(MSVC)
    target_compile_options(libtpumat PRIVATE
        /W4
        /WX         
        /wd4251
        /permissive-
        /Zc:__cplusplus
        /utf-8
    )
else()
    target_compile_options(libtpumat PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
endif()

install(TARGETS libtpumat
    EXPORT  libtpumatTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY libtpumat/
    DESTINATION include/libtpumat
    FILES_MATCHING PATTERN "*.hpp"
)
