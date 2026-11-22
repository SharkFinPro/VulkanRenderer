#ifndef VULKANPROJECT_RAYTRACER_H
#define VULKANPROJECT_RAYTRACER_H

#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace vke {

  class AssetManager;
  class Cloud;
  class DescriptorSet;
  class ImageResource;
  class LightingManager;
  class LogicalDevice;
  class Mesh;
  class Model;
  class PipelineManager;
  struct RenderInfo;
  class RenderObject;
  class Texture;
  class UniformBuffer;

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

  // One displayed chunk of a volume surface: its mesh (which must have a BLAS), the surface's model matrix and whether
  // it casts traced shadows.
  struct TracedVolumeChunk {
    const Mesh* mesh = nullptr;
    glm::mat4 modelMatrix{1.0f};
    bool castsShadows = true;
  };

  // What the volume closest hit shader reads through buffer references; 16 bytes, as std430 lays it out.
  struct VolumeChunkInfo {
    vk::DeviceAddress vertexAddress = 0;
    vk::DeviceAddress indexAddress = 0;
  };

  static_assert(sizeof(VolumeChunkInfo) == 16);

  class RayTracer {
  public:
    explicit RayTracer(std::shared_ptr<LogicalDevice> logicalDevice,
                       const std::shared_ptr<AssetManager>& assetManager,
                       vk::DescriptorPool descriptorPool);

    // False when there was nothing to trace, which leaves the images unwritten.
    [[nodiscard]] bool doRayTracing(const RenderInfo* renderInfo,
                                    const std::shared_ptr<PipelineManager>& pipelineManager,
                                    const std::shared_ptr<LightingManager>& lightingManager,
                                    const ImageResource& imageResource,
                                    const ImageResource& depthImageResource,
                                    const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                    const std::shared_ptr<Cloud>& cloud,
                                    std::span<const TracedVolumeChunk> volumeChunks);

  private:
    // Memory is declared first so that, on destruction, the buffer goes before the memory bound to it.
    struct DeviceBuffer {
      vk::raii::DeviceMemory memory = nullptr;
      vk::raii::Buffer buffer = nullptr;
    };

    // A host-visible, coherent buffer that stays mapped for its whole life.
    struct MappedBuffer {
      vk::raii::DeviceMemory memory = nullptr;
      vk::raii::Buffer buffer = nullptr;
      void* data = nullptr;
      vk::DeviceSize size = 0;
    };

    // Resources written every frame, one set per frame in flight. A set is only touched again once
    // FrameScheduler::beginFrame has waited for the frame that last used it, so it is rewritten in place. It
    // is only reallocated when the scene outgrows it, and the replaced buffers are retired.
    struct FrameResources {
      // Instances the TLAS buffers are sized for.
      uint32_t instanceCapacity = 0;
      MappedBuffer instances;
      vk::DeviceAddress instancesAddress = 0;

      DeviceBuffer tlasBuffer;
      vk::raii::AccelerationStructureKHR tlas = nullptr;
      vk::WriteDescriptorSetAccelerationStructureKHR tlasInfo{};

      DeviceBuffer scratch;
      vk::DeviceSize scratchSize = 0;
      vk::DeviceAddress scratchAddress = 0;

      MappedBuffer meshInfos;
      vk::DescriptorBufferInfo meshInfoInfo = { nullptr, 0, vk::WholeSize };

      MappedBuffer volumeChunkInfos;
      vk::DescriptorBufferInfo volumeChunkInfoInfo = { nullptr, 0, vk::WholeSize };
    };

    // Where a model's vertices and indices start in the merged buffers. The weak pointer tells a live
    // model from a new one allocated at a released model's address.
    struct ModelGeometry {
      std::weak_ptr<Model> model;
      uint32_t vertexOffset = 0;
      uint32_t indexOffset = 0;
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<FrameResources> m_frameResources;

    std::shared_ptr<UniformBuffer> m_cameraUniformRT;

    std::shared_ptr<DescriptorSet> m_rayTracingDescriptorSet;

    // Merged geometry of every model traced since the last rebuild, shared by all frames in flight. Rebuilt
    // only when a model it doesn't hold is traced; object transforms live in the TLAS instances instead.
    DeviceBuffer m_mergedVertexBuffer;
    DeviceBuffer m_mergedIndexBuffer;
    std::unordered_map<const Model*, ModelGeometry> m_modelGeometry;

    vk::DescriptorBufferInfo m_vertexBufferInfo = { nullptr, 0, vk::WholeSize };
    vk::DescriptorBufferInfo m_indexBufferInfo = { nullptr, 0, vk::WholeSize };

    // Parallel arrays rebuilt every frame; their capacity is kept, so a frame allocates nothing. The texture
    // array binding holds at most 256 textures, which keeps the linear lookup cheap.
    std::vector<vk::DescriptorImageInfo> m_textureImageInfos;
    std::vector<const Texture*> m_textureKeys;

    // Scratch for updateRTDescriptorSet, which runs every frame: 9 fixed bindings plus the texture array.
    std::array<vk::WriteDescriptorSet, 10> m_descriptorWrites{};

    std::shared_ptr<UniformBuffer> m_cloudUniform;

    // Orders this frame's TLAS build (and any merged geometry upload) before the trace.
    static void recordTraceInputBarrier(const RenderInfo* renderInfo,
                                        bool copiedGeometry);

    // Records a build of this frame's TLAS into the frame's command buffer.
    void buildTLAS(const RenderInfo* renderInfo,
                   const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                   const std::shared_ptr<Cloud>& cloud,
                   std::span<const TracedVolumeChunk> volumeChunks);

    void reserveTLAS(FrameResources& frame,
                     uint32_t instanceCount) const;

    [[nodiscard]] vk::AccelerationStructureBuildSizesInfoKHR getTLASBuildSizes(uint32_t instanceCount) const;

    void recreateTLAS(FrameResources& frame,
                      vk::DeviceSize size) const;

    void reserveScratch(FrameResources& frame,
                        vk::DeviceSize size) const;

    void populateInstances(vk::AccelerationStructureInstanceKHR* instances,
                           const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                           const std::shared_ptr<Cloud>& cloud,
                           std::span<const TracedVolumeChunk> volumeChunks) const;

    // Mask bit 0x01 is seen by camera and secondary rays, 0x02 casts traced shadows.
    [[nodiscard]] vk::AccelerationStructureInstanceKHR makeInstance(const glm::mat4& modelMatrix,
                                                                    vk::AccelerationStructureKHR blas,
                                                                    uint32_t customIndex,
                                                                    uint32_t hitGroupOffset,
                                                                    uint32_t mask = 0xFF,
                                                                    vk::GeometryInstanceFlagsKHR flags = {}) const;

    // Returns true when it recorded a copy into the merged geometry buffers.
    [[nodiscard]] bool updateMergedGeometry(const RenderInfo* renderInfo,
                                            const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    [[nodiscard]] bool mergedGeometryIsCurrent(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) const;

    [[nodiscard]] std::vector<std::shared_ptr<Model>> collectModelsToMerge(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) const;

    void rebuildMergedGeometry(const RenderInfo* renderInfo,
                               const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    // Records a copy of data into a new device-local storage buffer, through a staging buffer retired with
    // the frame.
    void uploadStorageBuffer(const RenderInfo* renderInfo,
                             const void* data,
                             vk::DeviceSize size,
                             DeviceBuffer& outBuffer) const;

    void updateMeshInfos(FrameResources& frame,
                         const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    void updateVolumeChunkInfos(FrameResources& frame,
                                std::span<const TracedVolumeChunk> volumeChunks) const;

    [[nodiscard]] uint32_t getTextureIndex(const std::shared_ptr<Texture>& texture);

    void reserveMappedBuffer(MappedBuffer& mappedBuffer,
                             vk::DeviceSize size,
                             vk::BufferUsageFlags usage) const;

    void updateRTDescriptorSet(const ImageResource& imageResource,
                               const ImageResource& depthImageResource,
                               uint32_t currentFrame);

    // Fills m_descriptorWrites for one frame's set and returns how many writes it holds.
    [[nodiscard]] uint32_t fillRTDescriptorWrites(vk::DescriptorSet descriptorSet,
                                                  const ImageResource& imageResource,
                                                  const ImageResource& depthImageResource,
                                                  uint32_t currentFrame);

    void updateRTDescriptorSetData(const RenderInfo* renderInfo);
  };
} // vke

#endif //VULKANPROJECT_RAYTRACER_H