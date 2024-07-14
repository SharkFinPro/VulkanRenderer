cmake_minimum_required(VERSION 3.27)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ENGINE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ENGINE_OUTPUT_DIRECTORY})

project(VulkanProject)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(source)

# Tests
include_directories(PUBLIC .)

link_libraries(VulkanEngine)

file(COPY "tests/assets/" DESTINATION "bin/assets/")

add_subdirectory(tests)