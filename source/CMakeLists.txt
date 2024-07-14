project(VulkanEngine)
include(FetchContent)

add_library(${PROJECT_NAME} SHARED
  VulkanEngine.cpp
  VulkanEngine.h
  Window.cpp
  Window.h
  DebugMessenger.cpp
  DebugMessenger.h
  objects/RenderObject.cpp
  objects/RenderObject.h
  Buffers.cpp
  Buffers.h
  objects/Model.cpp
  objects/Model.h
  objects/Texture.cpp
  objects/Texture.h
  Images.cpp
  Images.h
  Camera.cpp
  Camera.h
  objects/UniformBuffer.cpp
  objects/UniformBuffer.h
)

set(BUILD_SHARED_LIBS ON)

# Vulkan
FetchContent_Declare(
  vulkanHeaders
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
  GIT_TAG v1.3.290
)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)

# Assimp
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INSTALL_PDB OFF)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.4.2
)

# STB
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb
  GIT_TAG master
)

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
  FetchContent_Populate(stb)
  add_library(stb INTERFACE)
  target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()


# Include
FetchContent_MakeAvailable(
  glfw
  glm
  assimp
  stb
  vulkanHeaders
)

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
  Vulkan::Vulkan
  glfw
  assimp
  stb
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${glm_SOURCE_DIR}
)