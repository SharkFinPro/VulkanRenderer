#ifndef VULKANPROJECT_GUIPIPELINE_H
#define VULKANPROJECT_GUIPIPELINE_H

#include <array>
#include <vulkan/vulkan.h>
#include <memory>

#include "GraphicsPipeline.h"

class RenderPass;

class GuiPipeline final : public GraphicsPipeline {
public:
  GuiPipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
              const std::shared_ptr<RenderPass>& renderPass);
  ~GuiPipeline() override;

  void render(const VkCommandBuffer& commandBuffer, VkExtent2D swapChainExtent) const;

  VkDescriptorPool& getPool();

private:
  std::unique_ptr<VkPipelineColorBlendStateCreateInfo> defineColorBlendState() override;
  std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> defineDepthStencilState() override;
  std::unique_ptr<VkPipelineDynamicStateCreateInfo> defineDynamicState() override;
  std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> defineInputAssemblyState() override;
  std::unique_ptr<VkPipelineMultisampleStateCreateInfo> defineMultisampleState() override;
  std::unique_ptr<VkPipelineRasterizationStateCreateInfo> defineRasterizationState() override;
  std::unique_ptr<VkPipelineVertexInputStateCreateInfo> defineVertexInputState() override;
  std::unique_ptr<VkPipelineViewportStateCreateInfo> defineViewportState() override;

  void createDescriptorPool();

private:
  VkDescriptorPool descriptorPool;

  VkPipelineColorBlendAttachmentState colorBlendAttachment;

  std::array<VkDynamicState, 2> dynamicStates;

  VkVertexInputBindingDescription vertexBindingDescription;
  std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions;
};


#endif //VULKANPROJECT_GUIPIPELINE_H
