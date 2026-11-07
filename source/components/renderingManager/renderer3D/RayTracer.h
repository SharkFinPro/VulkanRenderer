#ifndef VULKANPROJECT_RAYTRACER_H
#define VULKANPROJECT_RAYTRACER_H

#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke {

  class AssetManager;
  class Cloud;
  class DescriptorSet;
  class ImageResource;
  class LightingManager;
  class LogicalDevice;
  class Model;
  class PipelineManager;
  struct RenderInfo;
  class RenderObject;
  class Texture;
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
                       vk::DescriptorPool descriptorPool);

    // False when there was nothing to trace, which leaves the images unwritten.
    [[nodiscard]] bool doRayTracing(const RenderInfo* renderInfo,
                                    const std::shared_ptr<PipelineManager>& pipelineManager,
                                    const std::shared_ptr<LightingManager>& lightingManager,
                                    const ImageResource& imageResource,
                                    const ImageResource& depthImageResource,
                                    const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                    const std::shared_ptr<Cloud>& cloud);

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
    // FrameScheduler::beginFrame has waited for the frame that last used it, so it is reused in place and
    // only reallocated when the scene outgrows it.
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

    // Scratch for updateRTDescriptorSet, which runs every frame: 8 fixed bindings plus the texture array.
    std::array<vk::WriteDescriptorSet, 9> m_descriptorWrites{};

    std::shared_ptr<UniformBuffer> m_cloudUniform;

    // Records a build of this frame's TLAS into the frame's command buffer.
    void buildTLAS(const RenderInfo* renderInfo,
                   const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                   const std::shared_ptr<Cloud>& cloud);

    void reserveTLAS(FrameResources& frame,
                     uint32_t instanceCount) const;

    void populateInstances(vk::AccelerationStructureInstanceKHR* instances,
                           const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                           const std::shared_ptr<Cloud>& cloud) const;

    // Returns true when it recorded a copy into the merged geometry buffers.
    [[nodiscard]] bool updateMergedGeometry(const RenderInfo* renderInfo,
                                            const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    [[nodiscard]] bool mergedGeometryIsCurrent(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) const;

    void rebuildMergedGeometry(const RenderInfo* renderInfo,
                               const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    void updateMeshInfos(FrameResources& frame,
                         const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    [[nodiscard]] uint32_t getTextureIndex(const std::shared_ptr<Texture>& texture);

    void reserveMappedBuffer(MappedBuffer& mappedBuffer,
                             vk::DeviceSize size,
                             vk::BufferUsageFlags usage) const;

    void updateRTDescriptorSet(const ImageResource& imageResource,
                               const ImageResource& depthImageResource,
                               uint32_t currentFrame);

    void updateRTDescriptorSetData(const RenderInfo* renderInfo);
  };
} // vke

#endif //VULKANPROJECT_RAYTRACER_H