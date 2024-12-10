#ifndef VULKANPROJECT_MODEL_H
#define VULKANPROJECT_MODEL_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <glm/vec3.hpp>

#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

struct Vertex;

class Model {
public:
  Model(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
        const VkCommandPool& commandPool, const char* path, glm::vec3 rotation);
  ~Model();

  void draw(const VkCommandBuffer& commandBuffer) const;

private:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;

  void loadModel(const char* path, glm::vec3 rotation);
  void createVertexBuffer(const VkCommandPool& commandPool);
  void createIndexBuffer(const VkCommandPool& commandPool);

  void bind(const VkCommandBuffer& commandBuffer) const;
};


#endif //VULKANPROJECT_MODEL_H
