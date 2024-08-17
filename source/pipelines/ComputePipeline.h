#ifndef VULKANPROJECT_COMPUTEPIPELINE_H
#define VULKANPROJECT_COMPUTEPIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>

#include "../components/LogicalDevice.h"
#include "../components/PhysicalDevice.h"

#include <array>
#include <glm/glm.hpp>

struct Particle {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec4 color;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Particle, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Particle, color);

    return attributeDescriptions;
  }
};

struct UniformBufferObject {
  float deltaTime = 1.0f;
};

constexpr int PARTICLE_COUNT = 8192;

class ComputePipeline {
public:
  ComputePipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
                  VkCommandPool& commandPool, VkRenderPass& renderPass, const VkExtent2D& swapChainExtent);
  ~ComputePipeline();

  void compute(const VkCommandBuffer& commandBuffer, uint32_t currentFrame) const;
  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, VkExtent2D swapChainExtent) const;

  void updateUniformBuffer(uint32_t currentFrame) const;

private:
  void createComputePipeline();

  void createGraphicsPipeline(VkRenderPass& renderPass);

  void createUniformBuffers();
  void createShaderStorageBuffers(VkCommandPool& commandPool, const VkExtent2D& swapChainExtent);

  void createDescriptorPool();
  void createDescriptorSets();

private:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  VkPipelineLayout computePipelineLayout;
  VkPipeline computePipeline;

  VkPipelineLayout graphicsPipelineLayout;
  VkPipeline graphicsPipeline;

  std::vector<VkBuffer> shaderStorageBuffers;
  std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;

  VkDescriptorSetLayout computeDescriptorSetLayout;
  VkDescriptorPool computeDescriptorPool;
  std::vector<VkDescriptorSet> computeDescriptorSets;

  float lastFrameTime = 0.25f;
};


#endif //VULKANPROJECT_COMPUTEPIPELINE_H
