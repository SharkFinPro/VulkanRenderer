cmake_minimum_required(VERSION 3.27)

option(VulkanProject_BUILD_TESTS "Enable building tests for VulkanProject" OFF)

project(VulkanProject)

if(APPLE)
  enable_language(OBJC)
endif()

set(CMAKE_CXX_STANDARD 23)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(VulkanProject_BUILD_TESTS ON)

  # Executables
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

  # Libraries (Shared and Static)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

  # Archive (Static Libraries)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
endif()

if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

if(UNIX)
  # Set the install RPATH to find libraries relative to the installed executable
  if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path")
  else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
  endif()

  # Ensure that the build executable works like the installed one
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

if (MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
endif()

add_subdirectory(source)

if (VulkanProject_BUILD_TESTS)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/tests/assets/" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/")

  add_subdirectory(tests)
endif()