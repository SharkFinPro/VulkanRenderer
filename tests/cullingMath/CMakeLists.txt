project("cullingMath")

add_executable(${PROJECT_NAME}
  main.cpp
  ${CMAKE_SOURCE_DIR}/source/utilities/Frustum.cpp
)

# The engine builds with this define; GLM reads it only the first time any of its headers is included.
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
