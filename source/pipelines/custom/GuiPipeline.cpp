#include "GuiPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../components/ImGuiInstance.h"
#include "../../components/core/logicalDevice/LogicalDevice.h"
#include <imgui.h>

GuiPipeline::GuiPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const std::shared_ptr<RenderPass>& renderPass, const uint32_t maxImGuiTextures)
  : GraphicsPipeline(logicalDevice)
{
  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/ui.vert.spv",
      .fragmentShader = "assets/shaders/ui.frag.spv"
    },
    .states {
      .colorBlendState = GraphicsPipelineStates::colorBlendState,
      .depthStencilState = GraphicsPipelineStates::depthStencilState,
      .dynamicState = GraphicsPipelineStates::dynamicState,
      .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
      .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
      .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
      .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
      .viewportState = GraphicsPipelineStates::viewportState
    },
    .renderPass = renderPass->getRenderPass()
  };

  createPipeline(graphicsPipelineOptions);

  createDescriptorPool(maxImGuiTextures);
}

GuiPipeline::~GuiPipeline()
{
  m_logicalDevice->destroyDescriptorPool(descriptorPool);
}

void GuiPipeline::render(const RenderInfo* renderInfo)
{
  GraphicsPipeline::render(renderInfo, nullptr);

  ImGui::Render();
  ImGuiInstance::renderDrawData(renderInfo->commandBuffer);
}

void GuiPipeline::createDescriptorPool(const uint32_t maxImGuiTextures)
{
  const std::array<VkDescriptorPoolSize, 11> poolSizes {
  {
    {VK_DESCRIPTOR_TYPE_SAMPLER, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_logicalDevice->getMaxFramesInFlight() * maxImGuiTextures},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, m_logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_logicalDevice->getMaxFramesInFlight()}
  }};

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    .maxSets = m_logicalDevice->getMaxFramesInFlight() * maxImGuiTextures,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  descriptorPool = m_logicalDevice->createDescriptorPool(poolCreateInfo);
}

VkDescriptorPool& GuiPipeline::getPool()
{
  return descriptorPool;
}
