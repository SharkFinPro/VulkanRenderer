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
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)

  # Libraries (Shared and Static)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)

  # Archive (Static Libraries)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)

  # The vke-profile preset. Profiling is a whole-build decision, so the option lives in the top-level block
  # and the flags are set before anything is added: every target, FetchContent dependencies included, gets
  # them. RelWithDebInfo brings the optimization and the debug info (a PDB beside each DLL and executable
  # on MSVC, DWARF elsewhere).
  option(VKE_PROFILE "Build optimized with debug info, frame pointers and linker settings that keep profiles readable" OFF)

  if (VKE_PROFILE)
    if (NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
      message(WARNING "VKE_PROFILE is meant for the RelWithDebInfo build type, not '${CMAKE_BUILD_TYPE}'.")
    endif()

    if (MSVC)
      # RelWithDebInfo inlines only functions marked inline (/Ob1), so hot paths would differ from Release;
      # /Ob2 matches Release. /DEBUG turns /OPT:REF off, leaving a larger and slower binary than Release, so
      # REF is set back on. ICF stays off, since folding identical functions makes a profiler attribute
      # samples to the wrong one. /INCREMENTAL:NO, because incremental linking conflicts with /OPT.
      string(REPLACE "/Ob1" "/Ob2" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
      string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
      add_link_options(/DEBUG /OPT:REF /OPT:NOICF /INCREMENTAL:NO)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      # Frame pointers let perf and other sampling profilers unwind without DWARF; -g comes from the build type.
      add_compile_options(-fno-omit-frame-pointer)
    else()
      message(WARNING "VKE_PROFILE is on but ${CMAKE_CXX_COMPILER_ID} has no profiling flags here; building without them.")
    endif()
  endif()
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