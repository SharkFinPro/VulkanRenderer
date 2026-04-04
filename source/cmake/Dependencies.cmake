# Vulkan
find_package(Vulkan REQUIRED)

add_compile_definitions(
  VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
  VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
)

# Configure and fetch third-party libraries
include(cmake/External.cmake)

# Set up dependency lists
set(VULKAN_ENGINE_LINK_LIBRARIES
  Vulkan::Vulkan
  glfw
  assimp
  imgui
  freetype
)

set(VULKAN_ENGINE_INCLUDE_DIRECTORIES
  ${glm_SOURCE_DIR}
  ${stb_SOURCE_DIR}
)