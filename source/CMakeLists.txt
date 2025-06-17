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
  pipelines/custom/ObjectsPipeline.cpp
  pipelines/custom/ObjectsPipeline.h
  pipelines/Vertex.h
  pipelines/RenderPass.cpp
  pipelines/RenderPass.h
  pipelines/custom/Uniforms.h
  pipelines/custom/GuiPipeline.cpp
  pipelines/custom/GuiPipeline.h
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
  pipelines/custom/DotsPipeline.cpp
  pipelines/custom/DotsPipeline.h
  pipelines/GraphicsPipeline.cpp
  pipelines/GraphicsPipeline.h
  pipelines/ComputePipeline.cpp
  pipelines/ComputePipeline.h
  pipelines/Pipeline.cpp
  pipelines/Pipeline.h
  objects/Light.cpp
  objects/Light.h
  pipelines/custom/EllipticalDots.cpp
  pipelines/custom/EllipticalDots.h
  pipelines/custom/NoisyEllipticalDots.cpp
  pipelines/custom/NoisyEllipticalDots.h
  objects/Noise3DTexture.cpp
  objects/Noise3DTexture.h
  pipelines/custom/CurtainPipeline.cpp
  pipelines/custom/CurtainPipeline.h
  pipelines/custom/BumpyCurtain.cpp
  pipelines/custom/BumpyCurtain.h
  pipelines/custom/CubeMapPipeline.cpp
  pipelines/custom/CubeMapPipeline.h
  objects/CubeMapTexture.cpp
  objects/CubeMapTexture.h
  pipelines/custom/TexturedPlane.cpp
  pipelines/custom/TexturedPlane.h
  pipelines/custom/GraphicsPipelineStates.h
  pipelines/Particle.h
  pipelines/custom/MagnifyWhirlMosaicPipeline.cpp
  pipelines/custom/MagnifyWhirlMosaicPipeline.h
  pipelines/custom/SnakePipeline.cpp
  pipelines/custom/SnakePipeline.h
  pipelines/custom/CrossesPipeline.cpp
  pipelines/custom/CrossesPipeline.h
  pipelines/custom/SmokePipeline.cpp
  pipelines/custom/SmokePipeline.h
  pipelines/SmokeParticle.h
)

# Noise
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/noise/" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/noise/")

# Cube Map
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/cubeMap/" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/cubeMap/")

# Shaders
set(shadersSrc ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(shadersDst ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/shaders)

file(MAKE_DIRECTORY ${shadersDst})

# Find all shader files recursively
file(GLOB_RECURSE SHADER_FILES
  "${shadersSrc}/*.vert"
  "${shadersSrc}/*.geom"
  "${shadersSrc}/*.frag"
  "${shadersSrc}/*.comp"
)

# List to store compiled SPIR-V shader files
set(SPV_FILES "")

foreach(SHADER ${SHADER_FILES})
  # Get the relative path of the shader file
  file(RELATIVE_PATH REL_PATH ${shadersSrc} ${SHADER})
  get_filename_component(DIR_PATH ${REL_PATH} DIRECTORY)
  get_filename_component(FILENAME ${SHADER} NAME)

  # Set the output SPIR-V file path
  set(SPV_FILE "${shadersDst}/${FILENAME}.spv")

  # Append to the list of SPIR-V files
  list(APPEND SPV_FILES ${SPV_FILE})

  # Add compilation command
  add_custom_command(
    OUTPUT ${SPV_FILE}
    COMMAND glslangValidator -V ${SHADER} -o ${SPV_FILE}
    DEPENDS ${SHADER}
    COMMENT "Compiling shader: ${REL_PATH}"
  )
endforeach()

# Define a custom target for shaders
add_custom_target(Shaders ALL DEPENDS ${SPV_FILES})

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
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v6.0.2
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
  GIT_TAG docking
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

if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL ${CMAKE_SOURCE_DIR}/source)
  #Create Include Headers
  file(COPY
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanEngine
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "include/*" EXCLUDE
  )
endif()