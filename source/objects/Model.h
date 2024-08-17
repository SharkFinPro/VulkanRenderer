#ifndef VULKANPROJECT_MODEL_H
#define VULKANPROJECT_MODEL_H

#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <glm/glm.hpp>

class Vertex;

class Model {
public:
  Model(VkDevice& device, VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue,
        const char* path);
  ~Model();

  void draw(const VkCommandBuffer& commandBuffer) const;

private:

  void loadModel(const char* path);
  void createVertexBuffer(const VkCommandPool& commandPool, const VkQueue& graphicsQueue);
  void createIndexBuffer(const VkCommandPool& commandPool, const VkQueue& graphicsQueue);

  void bind(const VkCommandBuffer& commandBuffer) const;

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
