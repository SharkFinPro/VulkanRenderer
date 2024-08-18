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
  pipelines/ObjectsPipeline.cpp
  pipelines/ObjectsPipeline.h
  pipelines/Vertex.cpp
  pipelines/Vertex.h
  pipelines/RenderPass.cpp
  pipelines/RenderPass.h
  pipelines/Uniforms.h
  pipelines/gui/GuiPipeline.cpp
  pipelines/gui/GuiPipeline.h
  pipelines/ShaderModule.cpp
  pipelines/ShaderModule.h
  components/PhysicalDevice.cpp
  components/PhysicalDevice.h
  components/Instance.cpp
  components/Instance.h
  components/LogicalDevice.cpp
  components/LogicalDevice.h
  VulkanEngineOptions.h
  components/ImGuiInstance.cpp
  components/ImGuiInstance.h
  components/SwapChain.cpp
  components/SwapChain.h
  components/Framebuffer.cpp
  components/Framebuffer.h
  pipelines/ComputePipeline.cpp
  pipelines/ComputePipeline.h
  pipelines/GraphicsPipeline.cpp
  pipelines/GraphicsPipeline.h
)

# Shaders
set(shadersSrc ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(shadersDst ${CMAKE_BINARY_DIR}/bin/assets/shaders)

file(MAKE_DIRECTORY ${shadersDst})

add_custom_target(Shaders ALL
  COMMAND glslangValidator -V ${shadersSrc}/shader.vert -o ${shadersDst}/vert.spv
  COMMAND glslangValidator -V ${shadersSrc}/shader.frag -o ${shadersDst}/frag.spv
  COMMAND glslangValidator -V ${shadersSrc}/ui/ui.vert -o ${shadersDst}/ui_vert.spv
  COMMAND glslangValidator -V ${shadersSrc}/ui/ui.frag -o ${shadersDst}/ui_frag.spv
  COMMAND glslangValidator -V ${shadersSrc}/compute/compute.vert -o ${shadersDst}/compute.vert.spv
  COMMAND glslangValidator -V ${shadersSrc}/compute/compute.frag -o ${shadersDst}/compute.frag.spv
  COMMAND glslangValidator -V ${shadersSrc}/compute/compute.comp -o ${shadersDst}/compute.comp.spv
  COMMENT "Compiling Shaders"
)

add_dependencies(${PROJECT_NAME} Shaders)

# Vulkan
find_package(Vulkan REQUIRED)

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

# imgui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.90.9
)

FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(imgui PUBLIC
  Vulkan::Vulkan
  glfw
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
)

# Include
FetchContent_MakeAvailable(
  glfw
  glm
  assimp
  stb
)

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