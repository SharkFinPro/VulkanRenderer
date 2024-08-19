#include "GuiPipeline.h"

#include <stdexcept>
#include <utility>

#include "../Vertex.h"
#include "../RenderPass.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

constexpr int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

GuiPipeline::GuiPipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
                         const std::shared_ptr<RenderPass>& renderPass)
  : GraphicsPipeline(std::move(physicalDevice), std::move(logicalDevice))
{
  createPipeline(renderPass->getRenderPass());

  createDescriptorPool();
}

GuiPipeline::~GuiPipeline()
{
  vkDestroyDescriptorPool(logicalDevice->getDevice(), descriptorPool, nullptr);
}

void GuiPipeline::render(const VkCommandBuffer& commandBuffer, const VkExtent2D swapChainExtent) const
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  const VkViewport viewport {
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(swapChainExtent.width),
    .height = static_cast<float>(swapChainExtent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  const VkRect2D scissor {
    .offset = {0, 0},
    .extent = swapChainExtent
  };
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer, nullptr);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void GuiPipeline::loadShaders()
{
  createShader("assets/shaders/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

std::unique_ptr<VkPipelineColorBlendStateCreateInfo> GuiPipeline::defineColorBlendState()
{
  colorBlendAttachment = {
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo colorBlendState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
  };

  return std::make_unique<VkPipelineColorBlendStateCreateInfo>(colorBlendState);
}

std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> GuiPipeline::defineDepthStencilState()
{
  VkPipelineDepthStencilStateCreateInfo depthStencilState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f
  };

  return std::make_unique<VkPipelineDepthStencilStateCreateInfo>(depthStencilState);
}

std::unique_ptr<VkPipelineDynamicStateCreateInfo> GuiPipeline::defineDynamicState()
{
  dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()
  };

  return std::make_unique<VkPipelineDynamicStateCreateInfo>(dynamicState);
}

std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> GuiPipeline::defineInputAssemblyState()
{
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  return std::make_unique<VkPipelineInputAssemblyStateCreateInfo>(inputAssemblyState);
}

std::unique_ptr<VkPipelineMultisampleStateCreateInfo> GuiPipeline::defineMultisampleState()
{
  VkPipelineMultisampleStateCreateInfo multisampleState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = physicalDevice->getMsaaSamples(),
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
    .pSampleMask = VK_NULL_HANDLE,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };

  return std::make_unique<VkPipelineMultisampleStateCreateInfo>(multisampleState);
}

std::unique_ptr<VkPipelineRasterizationStateCreateInfo> GuiPipeline::defineRasterizationState()
{
  VkPipelineRasterizationStateCreateInfo rasterizationState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f
  };

  return std::make_unique<VkPipelineRasterizationStateCreateInfo>(rasterizationState);
}

std::unique_ptr<VkPipelineVertexInputStateCreateInfo> GuiPipeline::defineVertexInputState()
{
  vertexBindingDescription = Vertex::getBindingDescription();
  vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
  };

  return std::make_unique<VkPipelineVertexInputStateCreateInfo>(vertexInputState);
}

std::unique_ptr<VkPipelineViewportStateCreateInfo> GuiPipeline::defineViewportState()
{
  VkPipelineViewportStateCreateInfo viewportState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1
  };

  return std::make_unique<VkPipelineViewportStateCreateInfo>(viewportState);
}

void GuiPipeline::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 11> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
  poolSizes[0].descriptorCount = 1000;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = 1000;
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  poolSizes[2].descriptorCount = 1000;
  poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[3].descriptorCount = 1000;
  poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
  poolSizes[4].descriptorCount = 1000;
  poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
  poolSizes[5].descriptorCount = 1000;
  poolSizes[6].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[6].descriptorCount = 1000;
  poolSizes[7].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSizes[7].descriptorCount = 1000;
  poolSizes[8].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  poolSizes[8].descriptorCount = 1000;
  poolSizes[9].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
  poolSizes[9].descriptorCount = 1000;
  poolSizes[10].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
  poolSizes[10].descriptorCount = 1000;

  VkDescriptorPoolCreateInfo poolCreateInfo{};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolCreateInfo.pPoolSizes = poolSizes.data();
  poolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(logicalDevice->getDevice(), &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

VkDescriptorPool& GuiPipeline::getPool()
{
  return descriptorPool;
}
