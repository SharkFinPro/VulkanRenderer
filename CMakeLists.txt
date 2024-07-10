cmake_minimum_required(VERSION 3.27)

# Engine Build Options
set(ENGINE_BUILD_TESTS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ENGINE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ENGINE_OUTPUT_DIRECTORY})

# Including Setup
include_directories(
  libraries/glfw-3.4.bin.WIN64/include
  libraries/glm
  C:/VulkanSDK/1.3.283.0/include
  libraries/stb
  libraries/assimp-5.4.2/include
)

# Linking Setup
link_directories(
  libraries/glfw-3.4.bin.WIN64/lib-mingw-w64
  libraries/assimp-5.4.2/lib
)

file(COPY "libraries/assimp-5.4.2/lib/libassimp-5.dll" DESTINATION "bin/")

set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.283.0")
find_package(Vulkan REQUIRED)

link_libraries(
  glfw3
  ${Vulkan_LIBRARIES}
  assimp
)

# Project Settings
project(RenderingProject)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(source)

# Tests
if (ENGINE_BUILD_TESTS)
  include_directories(PUBLIC .)

  link_libraries(VulkanEngine)

  file(COPY "tests/assets/" DESTINATION "bin/assets/")

  add_subdirectory(tests)
endif()