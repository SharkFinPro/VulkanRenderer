project("cullingMath")

# Relative to this file rather than CMAKE_SOURCE_DIR, which is the consumer's root when built through FetchContent
add_executable(${PROJECT_NAME}
  main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../source/utilities/Frustum.cpp
)

# The engine builds with this define; GLM reads it only the first time any of its headers is included.
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Header-only: glm::glm is a shared library here (BUILD_SHARED_LIBS), which the engine doesn't use either
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)
