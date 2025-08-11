# Core Components - Low-level API wrappers
set(VULKAN_ENGINE_SOURCES_CORE_COMPONENTS
  # Command Buffer Management
  components/core/commandBuffer/CommandBuffer.cpp
  components/core/commandBuffer/CommandBuffer.h

  # Instance Management
  components/core/instance/DebugMessenger.cpp
  components/core/instance/DebugMessenger.h
  components/core/instance/Instance.cpp
  components/core/instance/Instance.h

  # Logical Device Management
  components/core/logicalDevice/LogicalDevice.cpp
  components/core/logicalDevice/LogicalDevice.h

  # Physical Device Managements
  components/core/physicalDevice/PhysicalDevice.cpp
  components/core/physicalDevice/PhysicalDevice.h
)

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

  # Objects - Renderable objects and data structures
  components/objects/Model.cpp
  components/objects/Model.h
  components/objects/RenderObject.cpp
  components/objects/RenderObject.h

  # Rendering Manager
  components/renderingManager/LegacyRenderer.cpp
  components/renderingManager/LegacyRenderer.h
  components/renderingManager/Renderer.h
  components/renderingManager/RenderingManager.cpp
  components/renderingManager/RenderingManager.h

  # Textures
  components/textures/Texture.cpp
  components/textures/Texture.h
  components/textures/Texture2D.cpp
  components/textures/Texture2D.h
  components/textures/Texture3D.cpp
  components/textures/Texture3D.h
  components/textures/TextureCubemap.cpp
  components/textures/TextureCubemap.h

  # Windows
  components/window/SwapChain.cpp
  components/window/SwapChain.h
  components/window/Window.cpp
  components/window/Window.h

  # Core Components
  components/Camera.cpp
  components/Camera.h
  components/ImGuiInstance.cpp
  components/ImGuiInstance.h
  components/MousePicker.cpp
  components/MousePicker.h
  components/PipelineManager.cpp
  components/PipelineManager.h
  components/RenderPass.cpp
  components/RenderPass.h
  components/UniformBuffer.cpp
  components/UniformBuffer.h
)

# Pipeline Infrastructure
set(VULKAN_ENGINE_SOURCES_PIPELINES
  pipelines/ComputePipeline.cpp
  pipelines/ComputePipeline.h
  pipelines/GraphicsPipeline.cpp
  pipelines/GraphicsPipeline.h
  pipelines/Pipeline.cpp
  pipelines/Pipeline.h
  pipelines/ShaderModule.cpp
  pipelines/ShaderModule.h
)

# Custom Pipeline Implementations
set(VULKAN_ENGINE_SOURCES_PIPELINES_CUSTOM
  # RenderObject Pipelines
  pipelines/custom/renderObject/BumpyCurtain.cpp
  pipelines/custom/renderObject/BumpyCurtain.h
  pipelines/custom/renderObject/CrossesPipeline.cpp
  pipelines/custom/renderObject/CrossesPipeline.h
  pipelines/custom/renderObject/CubeMapPipeline.cpp
  pipelines/custom/renderObject/CubeMapPipeline.h
  pipelines/custom/renderObject/CurtainPipeline.cpp
  pipelines/custom/renderObject/CurtainPipeline.h
  pipelines/custom/renderObject/EllipticalDots.cpp
  pipelines/custom/renderObject/EllipticalDots.h
  pipelines/custom/renderObject/MagnifyWhirlMosaicPipeline.cpp
  pipelines/custom/renderObject/MagnifyWhirlMosaicPipeline.h
  pipelines/custom/renderObject/MousePickingPipeline.cpp
  pipelines/custom/renderObject/MousePickingPipeline.h
  pipelines/custom/renderObject/NoisyEllipticalDots.cpp
  pipelines/custom/renderObject/NoisyEllipticalDots.h
  pipelines/custom/renderObject/ObjectHighlightPipeline.cpp
  pipelines/custom/renderObject/ObjectHighlightPipeline.h
  pipelines/custom/renderObject/ObjectsPipeline.cpp
  pipelines/custom/renderObject/ObjectsPipeline.h
  pipelines/custom/renderObject/SnakePipeline.cpp
  pipelines/custom/renderObject/SnakePipeline.h
  pipelines/custom/renderObject/TexturedPlane.cpp
  pipelines/custom/renderObject/TexturedPlane.h

  # Other Pipelines
  pipelines/custom/BendyPipeline.cpp
  pipelines/custom/BendyPipeline.h
  pipelines/custom/DotsPipeline.cpp
  pipelines/custom/DotsPipeline.h
  pipelines/custom/GuiPipeline.cpp
  pipelines/custom/GuiPipeline.h
  pipelines/custom/LinePipeline.cpp
  pipelines/custom/LinePipeline.h
  pipelines/custom/SmokePipeline.cpp
  pipelines/custom/SmokePipeline.h

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
  ${VULKAN_ENGINE_SOURCES_CORE_COMPONENTS}
  ${VULKAN_ENGINE_SOURCES_COMPONENTS}
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