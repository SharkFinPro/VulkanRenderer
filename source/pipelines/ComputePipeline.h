#ifndef VULKANPROJECT_COMPUTEPIPELINE_H
#define VULKANPROJECT_COMPUTEPIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>

#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

class ComputePipeline {
public:
  ComputePipeline();
  ~ComputePipeline();

  void render(VkCommandBuffer& commandBuffer, uint32_t currentFrame);

private:
  void createPipeline();

private:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  std::vector<VkBuffer> shaderStorageBuffers;
  std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;
};


#endif //VULKANPROJECT_COMPUTEPIPELINE_H
