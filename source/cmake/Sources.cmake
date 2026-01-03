# Components - UI and rendering components
set(VULKAN_ENGINE_SOURCES_COMPONENTS
  # Asset Manager
    # Fonts
    components/assets/fonts/Font.cpp
    components/assets/fonts/Font.h

    # Objects - Renderable objects and data structures
    components/assets/objects/Model.cpp
    components/assets/objects/Model.h
    components/assets/objects/RenderObject.cpp
    components/assets/objects/RenderObject.h

    # Particle Systems
    components/assets/particleSystems/SmokeSystem.cpp
    components/assets/particleSystems/SmokeSystem.h

    # Textures
    components/assets/textures/Texture.cpp
    components/assets/textures/Texture.h
    components/assets/textures/Texture2D.cpp
    components/assets/textures/Texture2D.h
    components/assets/textures/Texture3D.cpp
    components/assets/textures/Texture3D.h
    components/assets/textures/TextureCubemap.cpp
    components/assets/textures/TextureCubemap.h
    components/assets/textures/TextureGlyph.cpp
    components/assets/textures/TextureGlyph.h

  components/assets/AssetManager.cpp
  components/assets/AssetManager.h

  # Camera
  components/camera/Camera.cpp
  components/camera/Camera.h

  # Command Buffer Management
  components/commandBuffer/CommandBuffer.cpp
  components/commandBuffer/CommandBuffer.h

  # Computing
  components/computingManager/ComputingManager.cpp
  components/computingManager/ComputingManager.h

  # ImGui
  components/imGui/ImGuiInstance.cpp
  components/imGui/ImGuiInstance.h

  # Instance Management
  components/instance/DebugMessenger.cpp
  components/instance/DebugMessenger.h
  components/instance/Instance.cpp
  components/instance/Instance.h

  # Lighting
    # Lights
    components/lighting/lights/Light.cpp
    components/lighting/lights/Light.h
    components/lighting/lights/PointLight.cpp
    components/lighting/lights/PointLight.h
    components/lighting/lights/SpotLight.cpp
    components/lighting/lights/SpotLight.h

  components/lighting/LightingManager.cpp
  components/lighting/LightingManager.h

  # Logical Device Management
  components/logicalDevice/LogicalDevice.cpp
  components/logicalDevice/LogicalDevice.h

  # Physical Device Management
  components/physicalDevice/PhysicalDevice.cpp
  components/physicalDevice/PhysicalDevice.h

  # Rendering Manager
    # Dynamic Renderer
    components/renderingManager/dynamicRenderer/DynamicRenderer.cpp
    components/renderingManager/dynamicRenderer/DynamicRenderer.h

    # Legacy Renderer
    components/renderingManager/legacyRenderer/LegacyRenderer.cpp
    components/renderingManager/legacyRenderer/LegacyRenderer.h
    components/renderingManager/legacyRenderer/Framebuffer.cpp
    components/renderingManager/legacyRenderer/Framebuffer.h
    components/renderingManager/legacyRenderer/RenderPass.cpp
    components/renderingManager/legacyRenderer/RenderPass.h

    # Renderer2D
    components/renderingManager/renderer2D/Renderer2D.cpp
    components/renderingManager/renderer2D/Renderer2D.h

    # Renderer3D
    components/renderingManager/renderer3D/MousePicker.cpp
    components/renderingManager/renderer3D/MousePicker.h
    components/renderingManager/renderer3D/Renderer3D.cpp
    components/renderingManager/renderer3D/Renderer3D.h

  components/renderingManager/ImageResource.cpp
  components/renderingManager/ImageResource.h
  components/renderingManager/Renderer.cpp
  components/renderingManager/Renderer.h
  components/renderingManager/RenderingManager.cpp
  components/renderingManager/RenderingManager.h
  components/renderingManager/RenderTarget.cpp
  components/renderingManager/RenderTarget.h

  # Windows
  components/window/Surface.cpp
  components/window/Surface.h
  components/window/SwapChain.cpp
  components/window/SwapChain.h
  components/window/Window.cpp
  components/window/Window.h
)

# Pipeline Infrastructure
set(VULKAN_ENGINE_SOURCES_PIPELINES
  components/pipelines/ComputePipeline.cpp
  components/pipelines/ComputePipeline.h
  components/pipelines/GraphicsPipeline.cpp
  components/pipelines/GraphicsPipeline.h
  components/pipelines/Pipeline.cpp
  components/pipelines/Pipeline.h

  # Descriptor Sets
  components/pipelines/descriptorSets/DescriptorSet.cpp
  components/pipelines/descriptorSets/DescriptorSet.h
  components/pipelines/descriptorSets/LayoutBindings.h

  # Pipeline Management
  components/pipelines/pipelineManager/PipelineManager.cpp
  components/pipelines/pipelineManager/PipelineManager.h

  # Shader Modules
  components/pipelines/shaderModules/ShaderModule.cpp
  components/pipelines/shaderModules/ShaderModule.h

  # Uniform Buffers
  components/pipelines/uniformBuffers/UniformBuffer.cpp
  components/pipelines/uniformBuffers/UniformBuffer.h
)

# implementations Pipeline Implementations
set(VULKAN_ENGINE_SOURCES_PIPELINES_IMPLEMENTATIONS
  # 2D Pipelines
  components/pipelines/implementations/2D/EllipsePipeline.cpp
  components/pipelines/implementations/2D/EllipsePipeline.h
  components/pipelines/implementations/2D/FontPipeline.cpp
  components/pipelines/implementations/2D/FontPipeline.h
  components/pipelines/implementations/2D/RectPipeline.cpp
  components/pipelines/implementations/2D/RectPipeline.h
  components/pipelines/implementations/2D/TrianglePipeline.cpp
  components/pipelines/implementations/2D/TrianglePipeline.h

  # RenderObject Pipelines
  components/pipelines/implementations/renderObject/BumpyCurtain.cpp
  components/pipelines/implementations/renderObject/BumpyCurtain.h
  components/pipelines/implementations/renderObject/CrossesPipeline.cpp
  components/pipelines/implementations/renderObject/CrossesPipeline.h
  components/pipelines/implementations/renderObject/CubeMapPipeline.cpp
  components/pipelines/implementations/renderObject/CubeMapPipeline.h
  components/pipelines/implementations/renderObject/CurtainPipeline.cpp
  components/pipelines/implementations/renderObject/CurtainPipeline.h
  components/pipelines/implementations/renderObject/EllipticalDots.cpp
  components/pipelines/implementations/renderObject/EllipticalDots.h
  components/pipelines/implementations/renderObject/MagnifyWhirlMosaicPipeline.cpp
  components/pipelines/implementations/renderObject/MagnifyWhirlMosaicPipeline.h
  components/pipelines/implementations/renderObject/MousePickingPipeline.cpp
  components/pipelines/implementations/renderObject/MousePickingPipeline.h
  components/pipelines/implementations/renderObject/NoisyEllipticalDots.cpp
  components/pipelines/implementations/renderObject/NoisyEllipticalDots.h
  components/pipelines/implementations/renderObject/ObjectHighlightPipeline.cpp
  components/pipelines/implementations/renderObject/ObjectHighlightPipeline.h
  components/pipelines/implementations/renderObject/ObjectsPipeline.cpp
  components/pipelines/implementations/renderObject/ObjectsPipeline.h
  components/pipelines/implementations/renderObject/PointLightShadowMapPipeline.cpp
  components/pipelines/implementations/renderObject/PointLightShadowMapPipeline.h
  components/pipelines/implementations/renderObject/ShadowPipeline.cpp
  components/pipelines/implementations/renderObject/ShadowPipeline.h
  components/pipelines/implementations/renderObject/SnakePipeline.cpp
  components/pipelines/implementations/renderObject/SnakePipeline.h
  components/pipelines/implementations/renderObject/TexturedPlane.cpp
  components/pipelines/implementations/renderObject/TexturedPlane.h

  # Other Pipelines
  components/pipelines/implementations/BendyPipeline.cpp
  components/pipelines/implementations/BendyPipeline.h
  components/pipelines/implementations/DotsPipeline.cpp
  components/pipelines/implementations/DotsPipeline.h
  components/pipelines/implementations/GridPipeline.cpp
  components/pipelines/implementations/GridPipeline.h
  components/pipelines/implementations/GuiPipeline.cpp
  components/pipelines/implementations/GuiPipeline.h
  components/pipelines/implementations/LinePipeline.cpp
  components/pipelines/implementations/LinePipeline.h
  components/pipelines/implementations/SmokePipeline.cpp
  components/pipelines/implementations/SmokePipeline.h

  # Configuration and State Headers
  components/pipelines/implementations/common/GraphicsPipelineStates.h
  components/pipelines/implementations/common/PipelineTypes.h
  components/pipelines/implementations/common/Uniforms.h

  # Vertex Input States
  components/pipelines/implementations/vertexInputs/LineVertex.h
  components/pipelines/implementations/vertexInputs/Particle.h
  components/pipelines/implementations/vertexInputs/SmokeParticle.h
  components/pipelines/implementations/vertexInputs/Vertex.h
)

# Utilities - Helper functions and common operations
set(VULKAN_ENGINE_SOURCES_UTILITIES
  utilities/Buffers.cpp
  utilities/Buffers.h
  utilities/EventSystem.h
  utilities/Images.cpp
  utilities/Images.h
)

# Base Engine Files
set(VULKAN_ENGINE_SOURCES_BASE
  VulkanEngine.cpp
  VulkanEngine.h
  EngineConfig.h
)

# Combine all source groups
set(VULKAN_ENGINE_SOURCES
  ${VULKAN_ENGINE_SOURCES_COMPONENTS}
  ${VULKAN_ENGINE_SOURCES_PIPELINES_IMPLEMENTATIONS}
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