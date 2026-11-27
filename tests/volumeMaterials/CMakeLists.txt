project("volumeMaterials")

# Header-only; included by repo-relative path
add_executable(${PROJECT_NAME} main.cpp)

# The shader whose constants the C++ mirror must match, read at run time
target_compile_definitions(${PROJECT_NAME} PRIVATE
  VKE_VOLUME_MATERIAL_GLSL="${CMAKE_CURRENT_SOURCE_DIR}/../../source/shaders/common/VolumeMaterial.glsl")

# Header-only: glm::glm is a shared library here (BUILD_SHARED_LIBS), which the engine does not use either
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)
