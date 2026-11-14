project("jobPool")

find_package(Threads REQUIRED)

# Relative to this file rather than CMAKE_SOURCE_DIR, which is the consumer's root when built through FetchContent
add_executable(${PROJECT_NAME}
  main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../source/utilities/JobPool.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
