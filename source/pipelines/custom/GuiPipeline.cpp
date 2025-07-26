#include "GuiPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../components/ImGuiInstance.h"
#include "../../core/commandBuffer/CommandBuffer.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include <imgui.h>

GuiPipeline::GuiPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const std::shared_ptr<RenderPass>& renderPass, const uint32_t maxImGuiTextures)
  : GraphicsPipeline(logicalDevice)
{
  createPipeline(renderPass->getRenderPass());

  createDescriptorPool(maxImGuiTextures);
}

GuiPipeline::~GuiPipeline()
{
  m_logicalDevice->destroyDescriptorPool(descriptorPool);
}

void GuiPipeline::render(const RenderInfo* renderInfo)
{
  GraphicsPipeline::render(renderInfo, nullptr);

  renderInfo->commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

  ImGui::Render();
  ImGuiInstance::renderDrawData(renderInfo->commandBuffer);
}

void GuiPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void GuiPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
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
