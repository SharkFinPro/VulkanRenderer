#ifndef VKE_MODEL_H
#define VKE_MODEL_H

#include "../../pipelines/implementations/vertexInputs/Vertex.h"
#include <assimp/mesh.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;

  class Model {
  public:
    Model(const std::shared_ptr<LogicalDevice>& logicalDevice,
          const vk::CommandPool& commandPool,
          const char* path,
          glm::vec3 rotation);

    Model(const std::shared_ptr<LogicalDevice>& logicalDevice,
          const vk::CommandPool& commandPool,
          const char* path,
          glm::quat orientation);

    void draw(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    [[nodiscard]] vk::AccelerationStructureKHR getBLAS() const;

    [[nodiscard]] const std::vector<Vertex>& getVertices() const;

    [[nodiscard]] const std::vector<uint32_t>& getIndices() const;

  private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    vk::raii::Buffer m_vertexBuffer = nullptr;
    vk::raii::DeviceMemory m_vertexBufferMemory = nullptr;

    vk::raii::Buffer m_indexBuffer = nullptr;
    vk::raii::DeviceMemory m_indexBufferMemory = nullptr;

    vk::raii::Buffer m_blasBuffer = nullptr;
    vk::raii::DeviceMemory m_blasBufferMemory = nullptr;
    vk::raii::AccelerationStructureKHR m_blas = nullptr;

    void loadModel(const char* path,
                   glm::quat orientation);

    void loadVertices(const aiMesh* mesh,
                      glm::quat orientation);

    void loadIndices(const aiMesh* mesh);

    void createVertexBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            const vk::CommandPool& commandPool);

    void createIndexBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const vk::CommandPool& commandPool);

    void bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void createBLAS(const std::shared_ptr<LogicalDevice>& logicalDevice,
                    const vk::CommandPool& commandPool);

    void createCoreBLASData(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            vk::AccelerationStructureGeometryTrianglesDataKHR& trianglesData,
                            vk::AccelerationStructureGeometryKHR& geometry,
                            vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo) const;

    void populateBLAS(const std::shared_ptr<LogicalDevice>& logicalDevice,
                      const vk::CommandPool& commandPool,
                      vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                      const vk::AccelerationStructureBuildSizesInfoKHR& buildSizesInfo,
                      uint32_t primitiveCount) const;
  };

} // namespace vke

#endif //VKE_MODEL_H
