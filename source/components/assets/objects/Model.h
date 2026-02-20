#ifndef VKE_MODEL_H
#define VKE_MODEL_H

#include <assimp/mesh.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  struct Vertex;

  class Model {
  public:
    Model(std::shared_ptr<LogicalDevice> logicalDevice,
          const VkCommandPool& commandPool,
          const char* path,
          glm::vec3 rotation);

    Model(std::shared_ptr<LogicalDevice> logicalDevice,
          const VkCommandPool& commandPool,
          const char* path,
          glm::quat orientation);

    ~Model();

    void draw(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    [[nodiscard]] VkAccelerationStructureKHR getBLAS() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_blasBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_blasBufferMemory = VK_NULL_HANDLE;
    VkAccelerationStructureKHR m_blas = VK_NULL_HANDLE;

    void loadModel(const char* path,
                   glm::quat orientation);

    void loadVertices(const aiMesh* mesh,
                      glm::quat orientation);

    void loadIndices(const aiMesh* mesh);

    void createVertexBuffer(const VkCommandPool& commandPool);

    void createIndexBuffer(const VkCommandPool& commandPool);

    void bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void createBLAS(const VkCommandPool& commandPool);

    void populateBLAS(const VkCommandPool& commandPool,
                      VkAccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                      const VkAccelerationStructureBuildSizesInfoKHR& buildSizesInfo,
                      uint32_t primitiveCount) const;
  };

} // namespace vke

#endif //VKE_MODEL_H
