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

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
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

constexpr int PARTICLE_COUNT = 1000;
constexpr int WIDTH = 600;
constexpr int HEIGHT = 400;

class ComputePipeline {
public:
  ComputePipeline(std::shared_ptr<PhysicalDevice> physicalDevice,
                  std::shared_ptr<LogicalDevice> logicalDevice,
                  VkCommandPool& commandPool);
  ~ComputePipeline();

  void compute(VkCommandBuffer& commandBuffer, uint32_t currentFrame);
  void render(VkCommandBuffer& commandBuffer, uint32_t currentFrame);

  void updateUniformBuffer(uint32_t currentFrame) const;

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

  float lastFrameTime = 0.0f;
};


#endif //VULKANPROJECT_COMPUTEPIPELINE_H
