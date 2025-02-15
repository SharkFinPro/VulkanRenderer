#ifndef VULKANPROJECT_GUIPIPELINE_H
#define VULKANPROJECT_GUIPIPELINE_H

#include "../GraphicsPipeline.h"
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

  void loadGraphicsShaders() override;

  void defineStates() override;

  void createDescriptorPool(uint32_t maxImGuiTextures);
};


#endif //VULKANPROJECT_GUIPIPELINE_H
