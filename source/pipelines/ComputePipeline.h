#ifndef VULKANPROJECT_COMPUTEPIPELINE_H
#define VULKANPROJECT_COMPUTEPIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>

#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

#include <glm/glm.hpp>

struct Particle {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;
};

struct UniformBufferObject {
  float deltaTime = 1.0f;
};

constexpr int PARTICLE_COUNT = 100;
constexpr int WIDTH = 600;
constexpr int HEIGHT = 400;

class ComputePipeline {
public:
  ComputePipeline(std::shared_ptr<PhysicalDevice> physicalDevice,
                  std::shared_ptr<LogicalDevice> logicalDevice,
                  VkCommandPool& commandPool);
  ~ComputePipeline();

  void render(VkCommandBuffer& commandBuffer, uint32_t currentFrame);

private:
  void createPipeline();

  void createUniformBuffers();
  void createShaderStorageBuffers(VkCommandPool& commandPool);

  void createDescriptorPool();
  void createDescriptorSets();

private:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;

  std::vector<VkBuffer> shaderStorageBuffers;
  std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
};


#endif //VULKANPROJECT_COMPUTEPIPELINE_H
