#ifndef VULKANPROJECT_GUIPIPELINE_H
#define VULKANPROJECT_GUIPIPELINE_H

#include "../GraphicsPipeline.h"
#include <array>
#include <vulkan/vulkan.h>
#include <memory>

class RenderPass;

class GuiPipeline final : public GraphicsPipeline {
public:
  GuiPipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
              const std::shared_ptr<RenderPass>& renderPass, uint32_t maxImGuiTextures);
  ~GuiPipeline() override;

  void render(const VkCommandBuffer& commandBuffer, VkExtent2D swapChainExtent) const;

  VkDescriptorPool& getPool();

private:
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment;

  std::array<VkDynamicState, 2> dynamicStates;

  VkVertexInputBindingDescription vertexBindingDescription;
  std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions;

  void loadGraphicsShaders() override;

  std::unique_ptr<VkPipelineColorBlendStateCreateInfo> defineColorBlendState() override;
  std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> defineDepthStencilState() override;
  std::unique_ptr<VkPipelineDynamicStateCreateInfo> defineDynamicState() override;
  std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> defineInputAssemblyState() override;
  std::unique_ptr<VkPipelineMultisampleStateCreateInfo> defineMultisampleState() override;
  std::unique_ptr<VkPipelineRasterizationStateCreateInfo> defineRasterizationState() override;
  std::unique_ptr<VkPipelineVertexInputStateCreateInfo> defineVertexInputState() override;
  std::unique_ptr<VkPipelineViewportStateCreateInfo> defineViewportState() override;

  void createDescriptorPool(uint32_t maxImGuiTextures);
};


#endif //VULKANPROJECT_GUIPIPELINE_H
