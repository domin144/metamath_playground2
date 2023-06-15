cmake_minimum_required(VERSION 3.2)

project(metamath_playground2)

find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 20)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-pedantic -Wall -Werror -Wno-sign-compare")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "-pedantic -Wall -Werror -Wno-sign-compare")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
endif()

add_executable(metamath_playground)
target_sources(
    metamath_playground
    PRIVATE
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
target_include_directories(metamath_playground PRIVATE src)
target_link_libraries(metamath_playground PRIVATE Boost::boost)
