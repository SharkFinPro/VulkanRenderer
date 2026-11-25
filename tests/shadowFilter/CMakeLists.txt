project("shadowFilter")

# Header-only; included by repo-relative path
add_executable(${PROJECT_NAME} main.cpp)

# The engine builds with this define; GLM reads it only the first time any of its headers is included.
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Header-only: glm::glm is a shared library here (BUILD_SHARED_LIBS), which the engine does not use either
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)
