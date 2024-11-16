cmake_minimum_required(VERSION 3.27)

option(VulkanRenderer_BUILD_TESTS "Enable building tests for VulkanRenderer" OFF)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(VulkanRenderer_BUILD_TESTS ON)

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
endif()

if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

project(VulkanProject)

set(CMAKE_CXX_STANDARD 23)

if (MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
endif()

add_subdirectory(source)

if (VulkanRenderer_BUILD_TESTS)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/tests/assets/" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/")

  add_subdirectory(tests)
endif()