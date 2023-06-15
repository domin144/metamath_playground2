include(ExternalProject)

ExternalProject_Add(
    adobe_source_libraries-external
    GIT_REPOSITORY https://github.com/stlab/adobe_source_libraries
    GIT_TAG master
    CMAKE_ARGS -DUSE_SYSTEM_BOOST=ON
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS <BINARY_DIR>/source/libasl.a
)

ExternalProject_Get_Property(adobe_source_libraries-external SOURCE_DIR BINARY_DIR)

add_library(adobe_source_libraries STATIC IMPORTED GLOBAL)
set_property(
    TARGET adobe_source_libraries
    PROPERTY IMPORTED_LOCATION
    ${BINARY_DIR}/source/libasl.a
)
target_include_directories(adobe_source_libraries INTERFACE ${SOURCE_DIR})
target_link_libraries(adobe_source_libraries INTERFACE Boost::boost)
add_dependencies(adobe_source_libraries adobe_source_libraries-external)
