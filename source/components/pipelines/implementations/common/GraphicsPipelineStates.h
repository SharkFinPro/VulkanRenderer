#ifndef VKE_GRAPHICSPIPELINESTATES_H
#define VKE_GRAPHICSPIPELINESTATES_H

#include "../vertexInputs/LineVertex.h"
#include "../vertexInputs/Particle.h"
#include "../vertexInputs/Vertex.h"
#include "../vertexInputs/SmokeParticle.h"
#include "../../../logicalDevice/LogicalDevice.h"
#include "../../../physicalDevice/PhysicalDevice.h"
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <memory>

namespace vke::GraphicsPipelineStates {

  inline vk::PipelineColorBlendAttachmentState colorBlendAttachment {
    .blendEnable = vk::False,
    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };

  inline vk::PipelineColorBlendStateCreateInfo colorBlendState {
    .logicOpEnable = vk::False,
    .logicOp = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants = {{ 0.0f, 0.0f, 0.0f, 0.0f }}
  };

  inline vk::PipelineColorBlendAttachmentState colorBlendAttachmentDots {
    .blendEnable = vk::True,
    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .colorBlendOp = vk::BlendOp::eAdd,
    .srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .alphaBlendOp = vk::BlendOp::eAdd,
    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };

  inline vk::PipelineColorBlendStateCreateInfo colorBlendStateDots {
    .logicOpEnable = vk::False,
    .logicOp = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachmentDots,
    .blendConstants = {{ 0.0f, 0.0f, 0.0f, 0.0f }}
  };

  inline vk::PipelineColorBlendAttachmentState colorBlendAttachmentSmoke {
    .blendEnable = vk::True,
    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .colorBlendOp = vk::BlendOp::eAdd,
    .srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstAlphaBlendFactor = vk::BlendFactor::eOne,
    .alphaBlendOp = vk::BlendOp::eAdd,
    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };

  inline vk::PipelineColorBlendStateCreateInfo colorBlendStateSmoke {
    .logicOpEnable = vk::False,
    .logicOp = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachmentSmoke,
    .blendConstants = {{ 0.0f, 0.0f, 0.0f, 0.0f }}
  };

  inline vk::PipelineColorBlendAttachmentState colorBlendAttachmentBendy {
    .blendEnable = vk::True,
    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .colorBlendOp = vk::BlendOp::eAdd,
    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
    .alphaBlendOp = vk::BlendOp::eAdd,
    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };

  inline vk::PipelineColorBlendStateCreateInfo colorBlendStateBendy {
    .logicOpEnable = vk::False,
    .logicOp = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachmentSmoke,
    .blendConstants = {{ 0.0f, 0.0f, 0.0f, 0.0f }}
  };

  inline vk::PipelineColorBlendStateCreateInfo colorBlendStateShadow {
    .logicOpEnable = vk::False,
    .attachmentCount = 0,
    .pAttachments = nullptr,
    .blendConstants = {{ 0.0f, 0.0f, 0.0f, 0.0f }}
  };

  inline vk::PipelineColorBlendAttachmentState colorBlendAttachmentTransparent {
    .blendEnable = vk::True,
    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    .colorBlendOp = vk::BlendOp::eAdd,
    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
    .alphaBlendOp = vk::BlendOp::eAdd,
    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
  };

  inline vk::PipelineColorBlendStateCreateInfo colorBlendStateTransparent {
    .logicOpEnable = vk::False,
    .logicOp = vk::LogicOp::eCopy,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachmentTransparent,
    .blendConstants = {{ 0.0f, 0.0f, 0.0f, 0.0f }}
  };

  inline vk::PipelineDepthStencilStateCreateInfo depthStencilState {
    .depthTestEnable = vk::True,
    .depthWriteEnable = vk::True,
    .depthCompareOp = vk::CompareOp::eLess,
    .depthBoundsTestEnable = vk::False,
    .stencilTestEnable = vk::False,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f
  };

  inline vk::PipelineDepthStencilStateCreateInfo depthStencilStateNone {
    .depthTestEnable = vk::False,
    .depthWriteEnable = vk::False
  };

  inline std::array dynamicStates {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  inline vk::PipelineDynamicStateCreateInfo dynamicState {
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()
  };

  inline vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateTriangleList {
    .topology = vk::PrimitiveTopology::eTriangleList,
    .primitiveRestartEnable = vk::False
  };

  inline vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateTriangleStrip {
    .topology = vk::PrimitiveTopology::eTriangleStrip,
    .primitiveRestartEnable = vk::False
  };

  inline vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStatePointList {
    .topology = vk::PrimitiveTopology::ePointList,
    .primitiveRestartEnable = vk::False
  };

  inline vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateLineList {
    .topology = vk::PrimitiveTopology::eLineList,
    .primitiveRestartEnable = vk::False
  };

  inline vk::PipelineMultisampleStateCreateInfo getMultsampleState(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    return {
      .rasterizationSamples = logicalDevice->getPhysicalDevice()->getMsaaSamples(),
      .sampleShadingEnable = vk::False,
      .minSampleShading = 1.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = vk::False,
      .alphaToOneEnable = vk::False
    };
  }

  inline vk::PipelineMultisampleStateCreateInfo getMultsampleStateAlpha(const std::shared_ptr<LogicalDevice>& logicalDevice)
  {
    return {
      .rasterizationSamples = logicalDevice->getPhysicalDevice()->getMsaaSamples(),
      .sampleShadingEnable = vk::False,
      .minSampleShading = 1.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = vk::True,
      .alphaToOneEnable = vk::False
    };
  }

  inline vk::PipelineMultisampleStateCreateInfo multisampleStateNone {
    .rasterizationSamples = vk::SampleCountFlagBits::e1,
    .sampleShadingEnable = vk::False,
    .minSampleShading = 1.0f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = vk::False,
    .alphaToOneEnable = vk::False
  };

  inline vk::PipelineRasterizationStateCreateInfo rasterizationStateCullBack {
    .depthClampEnable = vk::False,
    .rasterizerDiscardEnable = vk::False,
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eBack,
    .frontFace = vk::FrontFace::eCounterClockwise,
    .depthBiasEnable = vk::False,
    .lineWidth = 1.0f
  };

  inline vk::PipelineRasterizationStateCreateInfo rasterizationStateNoCull {
    .depthClampEnable = vk::False,
    .rasterizerDiscardEnable = vk::False,
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eNone,
    .frontFace = vk::FrontFace::eCounterClockwise,
    .depthBiasEnable = vk::False,
    .lineWidth = 1.0f
  };

  inline vk::PipelineRasterizationStateCreateInfo rasterizationStateOutline {
    .depthClampEnable = vk::False,
    .rasterizerDiscardEnable = vk::False,
    .polygonMode = vk::PolygonMode::eLine,
    .cullMode = vk::CullModeFlagBits::eNone,
    .frontFace = vk::FrontFace::eCounterClockwise,
    .depthBiasEnable = vk::False,
    .lineWidth = 1.0f
  };

  inline vk::PipelineVertexInputStateCreateInfo vertexInputStateRaw {
    .vertexBindingDescriptionCount = 0,
    .pVertexBindingDescriptions = nullptr,
    .vertexAttributeDescriptionCount = 0,
    .pVertexAttributeDescriptions = nullptr
  };

  inline vk::VertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
  inline std::array vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

  inline vk::PipelineVertexInputStateCreateInfo vertexInputStateVertex {
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
  };

  inline std::array vertexAttributeDescriptionsPositionOnly = Vertex::getAttributeDescriptionsPositionOnly();

  inline vk::PipelineVertexInputStateCreateInfo vertexInputStateVertexPositionOnly {
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptionsPositionOnly.size()),
    .pVertexAttributeDescriptions = vertexAttributeDescriptionsPositionOnly.data()
  };

  inline std::array vertexAttributeDescriptionsPositionAndNormal = Vertex::getAttributeDescriptionsPositionAndNormal();

  inline vk::PipelineVertexInputStateCreateInfo vertexInputStateVertexPositionAndNormal {
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptionsPositionAndNormal.size()),
    .pVertexAttributeDescriptions = vertexAttributeDescriptionsPositionAndNormal.data()
  };

  inline vk::VertexInputBindingDescription lineVertexBindingDescription = LineVertex::getBindingDescription();
  inline std::array lineVertexAttributeDescriptions = LineVertex::getAttributeDescriptions();

  inline vk::PipelineVertexInputStateCreateInfo vertexInputStateLineVertex {
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &lineVertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(lineVertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = lineVertexAttributeDescriptions.data()
  };

  inline vk::VertexInputBindingDescription particleBindingDescription = Particle::getBindingDescription();
  inline std::array particleAttributeDescriptions = Particle::getAttributeDescriptions();

  inline vk::PipelineVertexInputStateCreateInfo vertexInputStateParticle {
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &particleBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(particleAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = particleAttributeDescriptions.data()
  };

  inline vk::VertexInputBindingDescription smokeParticleBindingDescription = SmokeParticle::getBindingDescription();
  inline std::array smokeParticleAttributeDescriptions = SmokeParticle::getAttributeDescriptions();

  inline vk::PipelineVertexInputStateCreateInfo vertexInputStateSmokeParticle {
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &smokeParticleBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(smokeParticleAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = smokeParticleAttributeDescriptions.data()
  };

  inline vk::PipelineViewportStateCreateInfo viewportState {
    .viewportCount = 1,
    .scissorCount = 1
  };

} // namespace vke::GraphicsPipelineStates

#endif //VKE_GRAPHICSPIPELINESTATES_H