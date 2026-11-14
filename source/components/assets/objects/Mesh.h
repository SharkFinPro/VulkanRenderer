#ifndef VKE_MESH_H
#define VKE_MESH_H

#include "../../../utilities/Bounds.h"
#include <vulkan/vulkan_raii.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  class UploadQueue;
  struct UploadState;

  struct MeshCreateInfo {
    // Copied at creation; need not outlive the call.
    std::span<const std::byte> vertexData;
    // Bytes per vertex.
    uint32_t vertexStride = 0;
    // Byte offset of a float3 position inside each vertex, used for the bounds and the BLAS.
    uint32_t positionOffset = 0;
    // Triangle list.
    std::span<const uint32_t> indices;
    // Ignored when ray tracing is unsupported. Needs a stride and position offset that are multiples of four.
    bool buildBLAS = false;
  };

  // A lightweight drawable mesh with an arbitrary vertex format and no CPU copy of its data. It is uploaded through the
  // upload queue, so creating one never waits on the GPU; unlike Model it keeps no vertex or index arrays.
  class Mesh {
  public:
    // Throws std::invalid_argument on a stride of zero, a position that does not fit the stride, vertex data that is
    // empty or not a whole number of vertices, indices that are empty, not a multiple of three or out of range.
    Mesh(const std::shared_ptr<LogicalDevice>& logicalDevice,
         const std::shared_ptr<UploadQueue>& uploadQueue,
         const MeshCreateInfo& createInfo);

    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // True from the frame after the copies were recorded.
    [[nodiscard]] bool isReady() const;

    // Binds the buffers and draws instanceCount instances, as Model::draw does. Drawing a mesh that is not ready is
    // skipped, not an error.
    void draw(const std::shared_ptr<CommandBuffer>& commandBuffer,
              uint32_t firstInstance = 0,
              uint32_t instanceCount = 1) const;

    // Local-space bounds of the vertex positions.
    [[nodiscard]] const AABB& getBounds() const;

    [[nodiscard]] uint32_t getVertexStride() const;

    [[nodiscard]] uint32_t getVertexCount() const;

    [[nodiscard]] uint32_t getIndexCount() const;

    [[nodiscard]] bool hasBLAS() const;

    // Null unless built, and only meaningful once the mesh is ready.
    [[nodiscard]] vk::AccelerationStructureKHR getBLAS() const;

    [[nodiscard]] vk::Buffer getVertexBuffer() const;

    [[nodiscard]] vk::Buffer getIndexBuffer() const;

  private:
    // Retired together once the GPU may still use them; members are destroyed in reverse order, so the acceleration
    // structure goes before its buffer and each buffer before its memory.
    struct Resources {
      vk::raii::DeviceMemory vertexMemory = nullptr;
      vk::raii::Buffer vertexBuffer = nullptr;
      vk::raii::DeviceMemory indexMemory = nullptr;
      vk::raii::Buffer indexBuffer = nullptr;
      vk::raii::DeviceMemory blasMemory = nullptr;
      vk::raii::Buffer blasBuffer = nullptr;
      vk::raii::AccelerationStructureKHR blas = nullptr;
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<UploadState> m_state;

    Resources m_resources;

    AABB m_bounds;

    uint32_t m_vertexStride = 0;
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
  };

} // namespace vke

#endif //VKE_MESH_H
