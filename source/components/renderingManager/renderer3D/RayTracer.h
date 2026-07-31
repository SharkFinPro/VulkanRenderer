#ifndef VULKANPROJECT_RAYTRACER_H
#define VULKANPROJECT_RAYTRACER_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class AssetManager;
  class Cloud;
  class CommandBuffer;
  class DescriptorSet;
  class ImageResource;
  class LightingManager;
  class LogicalDevice;
  class PipelineManager;
  struct RenderInfo;
  class RenderObject;
  class UniformBuffer;
  struct Vertex;

  struct MeshInfo {
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    uint32_t textureIndex = 0;
    uint32_t specularIndex = 0;
    float reflectivity = 0.0f;
    float refractivity = 0.0f;
    float indexOfRefraction = 1.0f;
    float padding = 0.0f;
  };

  class RayTracer {
  public:
    explicit RayTracer(std::shared_ptr<LogicalDevice> logicalDevice,
                       const std::shared_ptr<AssetManager>& assetManager,
                       vk::CommandPool commandPool,
                       vk::DescriptorPool descriptorPool);

    void doRayTracing(const RenderInfo* renderInfo,
                      const std::shared_ptr<PipelineManager>& pipelineManager,
                      const std::shared_ptr<LightingManager>& lightingManager,
                      const ImageResource& imageResource,
                      const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                      const std::shared_ptr<Cloud>& cloud,
                      const glm::vec3& viewPosition,
                      const glm::mat4& viewMatrix);

  private:
    // Everything rebuilt per frame lives in a per-frame-in-flight slot. FrameScheduler::beginFrame()
    // has already waited for the frame that last used slot `currentFrame`, so the slot can be
    // rewritten with no further synchronization and nothing needs deferring.
    //
    // Buffers grow to a high-water mark and are then reused, so the steady state allocates
    // nothing: the TLAS is rebuilt in place into the same acceleration structure object, and the
    // instance and mesh-info buffers are host-visible and persistently mapped.
    struct FrameResources {
      vk::raii::AccelerationStructureKHR tlas = nullptr;
      vk::WriteDescriptorSetAccelerationStructureKHR tlasInfo {};

      vk::raii::Buffer tlasBuffer = nullptr;
      vk::raii::DeviceMemory tlasBufferMemory = nullptr;
      vk::DeviceSize tlasBufferSize = 0;

      vk::raii::Buffer scratchBuffer = nullptr;
      vk::raii::DeviceMemory scratchBufferMemory = nullptr;
      vk::DeviceSize scratchBufferSize = 0;

      // The instance array is small and fully rewritten every frame, so staging it through a
      // device-local copy would cost more than letting the build read host-visible memory.
      vk::raii::Buffer instanceBuffer = nullptr;
      vk::raii::DeviceMemory instanceBufferMemory = nullptr;
      void* instanceMapped = nullptr;
      vk::DeviceSize instanceBufferSize = 0;

      // MeshInfo carries per-frame-mutable material values, unlike the geometry buffers.
      vk::raii::Buffer meshInfoBuffer = nullptr;
      vk::raii::DeviceMemory meshInfoBufferMemory = nullptr;
      void* meshInfoMapped = nullptr;
      vk::DeviceSize meshInfoBufferSize = 0;
      vk::DescriptorBufferInfo meshInfoBufferInfo { nullptr, 0, vk::WholeSize };

      // Descriptors are only rewritten when a bound resource is actually replaced.
      bool descriptorsDirty = true;
      vk::Image boundStorageImage = nullptr;
    };

    // Buffers replaced mid-run (only when the scene's object set changes) cannot be freed
    // immediately, and staging buffers must outlive the frame's copy. Both are parked here and
    // released when the slot next comes around.
    struct RetiredResources {
      std::vector<vk::raii::Buffer> buffers;
      std::vector<vk::raii::DeviceMemory> memories;
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::CommandPool m_commandPool;

    std::vector<FrameResources> m_frames;
    std::vector<RetiredResources> m_retired;

    std::shared_ptr<UniformBuffer> m_cameraUniformRT;

    std::shared_ptr<DescriptorSet> m_rayTracingDescriptorSet;

    // Merged geometry is immutable for a given set of models, so it is shared across slots and
    // only re-uploaded when the scene's signature changes.
    vk::raii::Buffer m_mergedVertexBuffer = nullptr;
    vk::raii::DeviceMemory m_mergedVertexBufferMemory = nullptr;
    vk::DeviceSize m_mergedVertexBufferSize = 0;

    vk::raii::Buffer m_mergedIndexBuffer = nullptr;
    vk::raii::DeviceMemory m_mergedIndexBufferMemory = nullptr;
    vk::DeviceSize m_mergedIndexBufferSize = 0;

    vk::DescriptorBufferInfo m_vertexBufferInfo = { nullptr, 0, vk::WholeSize };
    vk::DescriptorBufferInfo m_indexBufferInfo = { nullptr, 0, vk::WholeSize };

    // Identifies the geometry currently uploaded: the ordered model/texture/specular pointers.
    std::vector<const void*> m_sceneSignature;

    // Mesh-info layout is derived from the scene; only the material values change per frame.
    std::vector<MeshInfo> m_meshInfos;

    std::vector<vk::DescriptorImageInfo> m_textureImageInfos;

    std::shared_ptr<UniformBuffer> m_cloudUniform;

    float m_speed = 1.0f;

    void createFrameResources();

    // Re-uploads merged vertices/indices and rebuilds the mesh-info layout when the object set has
    // changed, recording its copies into commandBuffer. Returns true if anything was replaced.
    bool updateSceneGeometry(const std::shared_ptr<CommandBuffer>& commandBuffer,
                             uint32_t currentFrame,
                             const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    void refreshMeshInfoMaterials(uint32_t currentFrame,
                                  FrameResources& frame,
                                  const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    void buildTLAS(const std::shared_ptr<CommandBuffer>& commandBuffer,
                   uint32_t currentFrame,
                   FrameResources& frame,
                   const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                   const std::shared_ptr<Cloud>& cloud);

    void populateInstanceArray(std::vector<vk::AccelerationStructureInstanceKHR>& instances,
                               const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                               const std::shared_ptr<Cloud>& cloud) const;

    void updateRTDescriptorSets(const ImageResource& imageResource,
                                uint32_t currentFrame,
                                FrameResources& frame);

    void updateRTDescriptorSetData(vk::Extent2D extent,
                                   uint32_t currentFrame,
                                   const glm::vec3& viewPosition,
                                   const glm::mat4& viewMatrix) const;

    // Grows buffer/memory to at least size, retiring the previous allocation. Returns true when a
    // new allocation was made, so anything holding the old handle must be refreshed.
    bool ensureBuffer(uint32_t currentFrame,
                      vk::DeviceSize size,
                      vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer& buffer,
                      vk::raii::DeviceMemory& memory,
                      vk::DeviceSize& currentSize,
                      void** mapped = nullptr);

    void retire(uint32_t currentFrame,
                vk::raii::Buffer&& buffer,
                vk::raii::DeviceMemory&& memory);
  };
} // vke

#endif //VULKANPROJECT_RAYTRACER_H
