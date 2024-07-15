#ifndef VULKANPROJECT_MODEL_H
#define VULKANPROJECT_MODEL_H

#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <glm/glm.hpp>

class Vertex;

class Model {
public:
  Model(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue,
        const char* path);
  ~Model();

  void draw(VkCommandBuffer& commandBuffer);

private:

  void loadModel(const char* path);
  void createVertexBuffer(VkCommandPool& commandPool, VkQueue& graphicsQueue);
  void createIndexBuffer(VkCommandPool& commandPool, VkQueue& graphicsQueue);

  void bind(VkCommandBuffer& commandBuffer);

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
};


#endif //VULKANPROJECT_MODEL_H
