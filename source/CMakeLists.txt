project(VulkanEngine)

# Set project-wide options
set(BUILD_SHARED_LIBS ON)

# Include modular CMake files
include(cmake/Assets.cmake)
include(cmake/CompileShaders.cmake)
include(cmake/Dependencies.cmake)
include(cmake/Sources.cmake)

# Create the main library
add_library(${PROJECT_NAME} ${VULKAN_ENGINE_SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC
  VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
  VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
  # Vulkan clips depth to 0..1, not OpenGL's -1..1. Defined for every translation unit, and publicly so code
  # built against the engine agrees, because GLM reads it only the first time any of its headers is included.
  GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Parse the heavy, stable third-party and standard headers once per build instead of once per source file.
# The tests reuse this header, so the library drops its <target>_EXPORTS define (nothing reads it; symbols are
# exported by WINDOWS_EXPORT_ALL_SYMBOLS) to keep its compile definitions identical to theirs.
set_target_properties(${PROJECT_NAME} PROPERTIES DEFINE_SYMBOL "")
target_precompile_headers(${PROJECT_NAME} PRIVATE
  <vulkan/vulkan_raii.hpp>
  <glm/vec3.hpp>
  <glm/mat4x4.hpp>
  <array>
  <memory>
  <stdexcept>
  <string>
  <vector>
)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC ${VULKAN_ENGINE_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_ENGINE_INCLUDE_DIRECTORIES})

# Add shader compilation dependency
add_dependencies(${PROJECT_NAME} Shaders)

# Handle header installation for library usage
include(cmake/Headers.cmake)