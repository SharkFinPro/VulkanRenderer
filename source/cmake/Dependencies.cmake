# Vulkan
find_package(Vulkan REQUIRED)

# Worker threads
find_package(Threads REQUIRED)

# Configure and fetch third-party libraries
include(cmake/External.cmake)

# Set up dependency lists
set(VULKAN_ENGINE_LINK_LIBRARIES
  Vulkan::Vulkan
  glfw
  assimp
  imgui
  freetype
  Threads::Threads
)

set(VULKAN_ENGINE_INCLUDE_DIRECTORIES
  ${glm_SOURCE_DIR}
  ${stb_SOURCE_DIR}
)