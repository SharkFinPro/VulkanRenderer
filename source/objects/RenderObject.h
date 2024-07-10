#ifndef VULKANPROJECT_RENDEROBJECT_H
#define VULKANPROJECT_RENDEROBJECT_H

#include <vulkan/vulkan.h>
#include <vector>
#include "Model.h"

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class RenderObject {
public:
  RenderObject(VkDevice& device, VkPhysicalDevice& physicalDevice, VkDescriptorSetLayout& descriptorSetLayout,
               VkImageView& textureImageView, VkSampler& textureSampler, Model* model);
  ~RenderObject();

  void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t currentFrame);

  void updateUniformBuffer(uint32_t currentFrame, VkExtent2D& swapChainExtent);

private:

  void createUniformBuffers();

  void createDescriptorPool();
  void createDescriptorSets(VkDescriptorSetLayout& descriptorSetLayout, VkImageView& textureImageView,
                            VkSampler& textureSampler);

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  Model* model;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
};


#endif //VULKANPROJECT_RENDEROBJECT_H
