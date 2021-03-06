cmake_minimum_required(VERSION 3.2)

project(metamath_playground2)

set(
    SOURCES
    src/metamath_database.cpp
    src/metamath_database.h
    src/metamath_database_read_write.cpp
    src/metamath_database_read_write.h
    src/metamath_playground.cpp
    src/named.h
    src/tokenizer.cpp
    src/tokenizer.h
    src/typed_indices.h
    src/adobe/config/compiler/visualc.hpp
    src/adobe/config/compiler/gcc.hpp
    src/adobe/config/select_compiler.hpp
    src/adobe/config.hpp
    src/adobe/forest.hpp
    src/adobe/iterator/set_next.hpp
    src/adobe/algorithm/reverse.hpp)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-std=c++14 -pedantic -Wall -Werror -Wno-sign-compare")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-std=c++14 -pedantic -Wall -Werror -Wno-sign-compare")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
endif()

include_directories(src)

add_executable(metamath_playground ${SOURCES})
