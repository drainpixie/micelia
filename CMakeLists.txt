cmake_minimum_required(VERSION 3.10)
project(micelia)

set(CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER clang++)
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")


file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")
add_executable(micelia ${SOURCES})

find_package(fmt REQUIRED)
target_link_libraries(micelia fmt::fmt)
