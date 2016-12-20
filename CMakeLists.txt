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
        src/typed_indices.h)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-std=c++14 -pedantic -Wall -Werror -Wno-sign-compare")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-std=c++14 -pedantic -Wall -Werror -Wno-sign-compare")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
endif()

add_executable(metamath_playground ${SOURCES})
