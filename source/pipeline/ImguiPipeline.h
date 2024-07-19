#ifndef VULKANPROJECT_IMGUIPIPELINE_H
#define VULKANPROJECT_IMGUIPIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class RenderPass;

class ImguiPipeline {
public:
  ImguiPipeline(VkDevice& device, VkPhysicalDevice& physicalDevice, const char* vertexShader,
                const char* fragmentShader, VkExtent2D& swapChainExtent,
                VkSampleCountFlagBits msaaSamples, std::shared_ptr<RenderPass> renderPass);
  ~ImguiPipeline();

  void render(VkCommandBuffer& commandBuffer);

  VkDescriptorPool& getPool();

private:
  void createGraphicsPipeline(const char* vertexShader, const char* fragmentShader, VkSampleCountFlagBits msaaSamples,
                              std::shared_ptr<RenderPass>& renderPass);

  VkShaderModule createShaderModule(const char* file);

  void createDescriptorPool();

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkExtent2D& swapChainExtent;

  VkDescriptorPool descriptorPool;
};


#endif //VULKANPROJECT_IMGUIPIPELINE_H
