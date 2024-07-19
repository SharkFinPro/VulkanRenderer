project(VulkanEngine)
include(FetchContent)

set(BUILD_SHARED_LIBS ON)

add_library(${PROJECT_NAME}
  VulkanEngine.cpp
  VulkanEngine.h
  components/Window.cpp
  components/Window.h
  components/DebugMessenger.cpp
  components/DebugMessenger.h
  objects/RenderObject.cpp
  objects/RenderObject.h
  utilities/Buffers.cpp
  utilities/Buffers.h
  objects/Model.cpp
  objects/Model.h
  objects/Texture.cpp
  objects/Texture.h
  utilities/Images.cpp
  utilities/Images.h
  components/Camera.cpp
  components/Camera.h
  objects/UniformBuffer.cpp
  objects/UniformBuffer.h
  pipeline/GraphicsPipeline.cpp
  pipeline/GraphicsPipeline.h
  pipeline/Vertex.cpp
  pipeline/Vertex.h
  pipeline/RenderPass.cpp
  pipeline/RenderPass.h
  pipeline/Framebuffer.cpp
  pipeline/Framebuffer.h
  pipeline/Uniforms.h
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

# imgui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.90.9
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
  add_library(imgui INTERFACE)
  target_include_directories(imgui INTERFACE ${imgui_SOURCE_DIR})
endif()


# Include
FetchContent_MakeAvailable(
  glfw
  glm
  assimp
  stb
  imgui
)

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
  Vulkan::Vulkan
  glfw
  assimp
  imgui
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${glm_SOURCE_DIR}
  ${stb_SOURCE_DIR}
)