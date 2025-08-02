#ifndef VULKANPROJECT_MODEL_H
#define VULKANPROJECT_MODEL_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

struct aiMesh;
class CommandBuffer;
class LogicalDevice;
struct Vertex;

class Model {
public:
  Model(const std::shared_ptr<LogicalDevice>& logicalDevice,
        const VkCommandPool& commandPool,
        const char* path,
        glm::vec3 rotation);

  Model(const std::shared_ptr<LogicalDevice>& logicalDevice,
        const VkCommandPool& commandPool,
        const char* path,
        glm::quat orientation);

  ~Model();

  void draw(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;

  VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

  VkBuffer m_indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

  void loadModel(const char* path, glm::quat orientation);

  void loadVertices(const aiMesh* mesh, glm::quat orientation);

  void loadIndices(const aiMesh* mesh);

  void createVertexBuffer(const VkCommandPool& commandPool);

  void createIndexBuffer(const VkCommandPool& commandPool);

  void bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const;
};


#endif //VULKANPROJECT_MODEL_H
