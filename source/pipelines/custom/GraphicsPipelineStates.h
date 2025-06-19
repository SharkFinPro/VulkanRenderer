#ifndef GRAPHICSPIPELINESTATES_H
#define GRAPHICSPIPELINESTATES_H

#include "../LineVertex.h"
#include "../Particle.h"
#include "../Vertex.h"
#include "../SmokeParticle.h"
#include "../../components/PhysicalDevice.h"
#include <vulkan/vulkan.h>
#include <array>
#include <memory>

namespace GraphicsPipelineStates {
  inline VkPipelineColorBlendAttachmentState colorBlendAttachment {
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  inline VkPipelineColorBlendStateCreateInfo colorBlendState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
  };

  inline VkPipelineColorBlendAttachmentState colorBlendAttachmentDots {
    .blendEnable = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  inline VkPipelineColorBlendStateCreateInfo colorBlendStateDots {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachmentDots,
    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
  };

  inline VkPipelineColorBlendAttachmentState colorBlendAttachmentSmoke {
    .blendEnable = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  inline VkPipelineColorBlendStateCreateInfo colorBlendStateSmoke {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachmentSmoke,
    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
  };

  inline VkPipelineDepthStencilStateCreateInfo depthStencilState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f
  };

  inline VkPipelineDepthStencilStateCreateInfo depthStencilStateNone {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_FALSE,
    .depthWriteEnable = VK_FALSE
  };

  inline std::array dynamicStates {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  inline VkPipelineDynamicStateCreateInfo dynamicState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()
  };

  inline VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateTriangleList {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  inline VkPipelineInputAssemblyStateCreateInfo inputAssemblyStatePointList {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  inline VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateLineList {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  inline VkPipelineMultisampleStateCreateInfo getMultsampleState(const std::shared_ptr<PhysicalDevice>& physicalDevice)
  {
    return {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = physicalDevice->getMsaaSamples(),
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 1.0f,
      .pSampleMask = VK_NULL_HANDLE,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE
    };
  }

  inline VkPipelineRasterizationStateCreateInfo rasterizationStateCullBack {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f
  };

  inline VkPipelineRasterizationStateCreateInfo rasterizationStateNoCull {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f
  };

  inline VkVertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
  inline std::array vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

  inline VkPipelineVertexInputStateCreateInfo vertexInputStateVertex {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
  };

  inline VkVertexInputBindingDescription lineVertexBindingDescription = LineVertex::getBindingDescription();
  inline std::array lineVertexAttributeDescriptions = LineVertex::getAttributeDescriptions();

  inline VkPipelineVertexInputStateCreateInfo vertexInputStateLineVertex {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &lineVertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(lineVertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = lineVertexAttributeDescriptions.data()
  };

  inline VkVertexInputBindingDescription particleBindingDescription = Particle::getBindingDescription();
  inline std::array particleAttributeDescriptions = Particle::getAttributeDescriptions();

  inline VkPipelineVertexInputStateCreateInfo vertexInputStateParticle {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &particleBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(particleAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = particleAttributeDescriptions.data()
  };

  inline VkVertexInputBindingDescription smokeParticleBindingDescription = SmokeParticle::getBindingDescription();
  inline std::array smokeParticleAttributeDescriptions = SmokeParticle::getAttributeDescriptions();

  inline VkPipelineVertexInputStateCreateInfo vertexInputStateSmokeParticle {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &smokeParticleBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(smokeParticleAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = smokeParticleAttributeDescriptions.data()
  };

  inline VkPipelineViewportStateCreateInfo viewportState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1
  };
}

#endif //GRAPHICSPIPELINESTATES_H
