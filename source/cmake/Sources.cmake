# Components - UI and rendering components
set(VULKAN_ENGINE_SOURCES_COMPONENTS
  # Framebuffers
  components/framebuffers/Framebuffer.cpp
  components/framebuffers/Framebuffer.h
  components/framebuffers/StandardFramebuffer.cpp
  components/framebuffers/StandardFramebuffer.h
  components/framebuffers/SwapchainFramebuffer.cpp
  components/framebuffers/SwapchainFramebuffer.h

  # Lighting
  components/lighting/Light.cpp
  components/lighting/Light.h
  components/lighting/LightingManager.cpp
  components/lighting/LightingManager.h

  # Textures
  components/textures/Texture.cpp
  components/textures/Texture.h
  components/textures/Texture2D.cpp
  components/textures/Texture2D.h
  components/textures/Texture3D.cpp
  components/textures/Texture3D.h
  components/textures/TextureCubemap.cpp
  components/textures/TextureCubemap.h

  # Core Components
  components/Camera.cpp
  components/Camera.h
  components/ImGuiInstance.cpp
  components/ImGuiInstance.h
  components/MousePicker.cpp
  components/MousePicker.h
  components/SwapChain.cpp
  components/SwapChain.h
  components/Window.cpp
  components/Window.h
)

# Core - Low-level Vulkan API wrappers
set(VULKAN_ENGINE_SOURCES_CORE
  # Command Buffer Management
  core/commandBuffer/CommandBuffer.cpp
  core/commandBuffer/CommandBuffer.h

  # Instance Management
  core/instance/DebugMessenger.cpp
  core/instance/DebugMessenger.h
  core/instance/Instance.cpp
  core/instance/Instance.h

  # Logical Device Management
  core/logicalDevice/LogicalDevice.cpp
  core/logicalDevice/LogicalDevice.h

  # Physical Device Managements
  core/physicalDevice/PhysicalDevice.cpp
  core/physicalDevice/PhysicalDevice.h
)

# Objects - Renderable objects and data structures
set(VULKAN_ENGINE_SOURCES_OBJECTS
  objects/Model.cpp
  objects/Model.h
  objects/RenderObject.cpp
  objects/RenderObject.h
  objects/UniformBuffer.cpp
  objects/UniformBuffer.h
)

# Pipeline Infrastructure
set(VULKAN_ENGINE_SOURCES_PIPELINES
  pipelines/ComputePipeline.cpp
  pipelines/ComputePipeline.h
  pipelines/GraphicsPipeline.cpp
  pipelines/GraphicsPipeline.h
  pipelines/Pipeline.cpp
  pipelines/Pipeline.h
  pipelines/RenderPass.cpp
  pipelines/RenderPass.h
  pipelines/ShaderModule.cpp
  pipelines/ShaderModule.h
)

# Custom Pipeline Implementations
set(VULKAN_ENGINE_SOURCES_PIPELINES_CUSTOM
  pipelines/custom/BendyPipeline.cpp
  pipelines/custom/BendyPipeline.h
  pipelines/custom/BumpyCurtain.cpp
  pipelines/custom/BumpyCurtain.h
  pipelines/custom/CrossesPipeline.cpp
  pipelines/custom/CrossesPipeline.h
  pipelines/custom/CubeMapPipeline.cpp
  pipelines/custom/CubeMapPipeline.h
  pipelines/custom/CurtainPipeline.cpp
  pipelines/custom/CurtainPipeline.h
  pipelines/custom/DotsPipeline.cpp
  pipelines/custom/DotsPipeline.h
  pipelines/custom/EllipticalDots.cpp
  pipelines/custom/EllipticalDots.h
  pipelines/custom/GuiPipeline.cpp
  pipelines/custom/GuiPipeline.h
  pipelines/custom/LinePipeline.cpp
  pipelines/custom/LinePipeline.h
  pipelines/custom/MagnifyWhirlMosaicPipeline.cpp
  pipelines/custom/MagnifyWhirlMosaicPipeline.h
  pipelines/custom/MousePickingPipeline.cpp
  pipelines/custom/MousePickingPipeline.h
  pipelines/custom/NoisyEllipticalDots.cpp
  pipelines/custom/NoisyEllipticalDots.h
  pipelines/custom/ObjectHighlightPipeline.cpp
  pipelines/custom/ObjectHighlightPipeline.h
  pipelines/custom/ObjectsPipeline.cpp
  pipelines/custom/ObjectsPipeline.h
  pipelines/custom/SmokePipeline.cpp
  pipelines/custom/SmokePipeline.h
  pipelines/custom/SnakePipeline.cpp
  pipelines/custom/SnakePipeline.h
  pipelines/custom/TexturedPlane.cpp
  pipelines/custom/TexturedPlane.h

  # Configuration and State Headers
  pipelines/custom/config/GraphicsPipelineStates.h
  pipelines/custom/config/PipelineTypes.h
  pipelines/custom/config/Uniforms.h
  pipelines/custom/descriptorSets/DescriptorSet.cpp
  pipelines/custom/descriptorSets/DescriptorSet.h
  pipelines/custom/descriptorSets/LayoutBindings.h

  # Vertex Input States
  pipelines/custom/vertexInputs/LineVertex.h
  pipelines/custom/vertexInputs/Particle.h
  pipelines/custom/vertexInputs/SmokeParticle.h
  pipelines/custom/vertexInputs/Vertex.h
)

# Utilities - Helper functions and common operations
set(VULKAN_ENGINE_SOURCES_UTILITIES
  utilities/Buffers.cpp
  utilities/Buffers.h
  utilities/Images.cpp
  utilities/Images.h
)

# Base Engine Files
set(VULKAN_ENGINE_SOURCES_BASE
  VulkanEngine.cpp
  VulkanEngine.h
  VulkanEngineOptions.h
)

# Combine all source groups
set(VULKAN_ENGINE_SOURCES
  ${VULKAN_ENGINE_SOURCES_COMPONENTS}
  ${VULKAN_ENGINE_SOURCES_CORE}
  ${VULKAN_ENGINE_SOURCES_OBJECTS}
  ${VULKAN_ENGINE_SOURCES_PIPELINES_CUSTOM}
  ${VULKAN_ENGINE_SOURCES_PIPELINES}
  ${VULKAN_ENGINE_SOURCES_UTILITIES}
  ${VULKAN_ENGINE_SOURCES_BASE}
)

# Validation: Ensure all listed files exist
foreach(SOURCE_FILE ${VULKAN_ENGINE_SOURCES})
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}")
    message(WARNING "Source file not found: ${SOURCE_FILE}")
  endif()
endforeach()