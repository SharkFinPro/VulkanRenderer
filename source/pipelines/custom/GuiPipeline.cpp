#include "GuiPipeline.h"
#include "GraphicsPipelineStates.h"
#include "../RenderPass.h"
#include "../../components/LogicalDevice.h"
#include "../../components/PhysicalDevice.h"
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <stdexcept>

GuiPipeline::GuiPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                         const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const std::shared_ptr<RenderPass>& renderPass, const uint32_t maxImGuiTextures)
  : GraphicsPipeline(physicalDevice, logicalDevice)
{
  createPipeline(renderPass->getRenderPass());

  createDescriptorPool(maxImGuiTextures);
}

GuiPipeline::~GuiPipeline()
{
  logicalDevice->destroyDescriptorPool(descriptorPool);
}

void GuiPipeline::render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects)
{
  GraphicsPipeline::render(renderInfo, objects);

  vkCmdBindPipeline(renderInfo->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), renderInfo->commandBuffer, nullptr);
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
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(physicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void GuiPipeline::createDescriptorPool(const uint32_t maxImGuiTextures)
{
  const std::array<VkDescriptorPoolSize, 11> poolSizes {
  {
    {VK_DESCRIPTOR_TYPE_SAMPLER, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, logicalDevice->getMaxFramesInFlight() * maxImGuiTextures},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, logicalDevice->getMaxFramesInFlight()},
    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, logicalDevice->getMaxFramesInFlight()}
  }};

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    .maxSets = logicalDevice->getMaxFramesInFlight() * maxImGuiTextures,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  descriptorPool = logicalDevice->createDescriptorPool(poolCreateInfo);
}

VkDescriptorPool& GuiPipeline::getPool()
{
  return descriptorPool;
}
