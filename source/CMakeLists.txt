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
)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC ${VULKAN_ENGINE_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_ENGINE_INCLUDE_DIRECTORIES})

# Add shader compilation dependency
add_dependencies(${PROJECT_NAME} Shaders)

# Handle header installation for library usage
include(cmake/Headers.cmake)