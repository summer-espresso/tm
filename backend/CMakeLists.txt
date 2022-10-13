cmake_minimum_required(VERSION 3.0)

project(tm)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

include_directories(
    ${jsoncpp_INCLUDE_DIRS}
    lib/Crow/include
    /usr/include/jsoncpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

file(
    GLOB_RECURSE
    source_files
    src/*
)

add_executable(
    tm
    ${source_files}
)

set_property(TARGET tm PROPERTY CXX_STANDARD 11)

target_link_libraries(
    tm
    ${JSONCPP_LIBRARIES}
    pthread
    curl
    procps
)

install(TARGETS tm RUNTIME DESTINATION bin)
