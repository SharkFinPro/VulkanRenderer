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
  class DescriptorSet;
  class ImageResource;
  class LightingManager;
  class LogicalDevice;
  class PipelineManager;
  struct RenderInfo;
  class RenderObject;
  class SmokeVolume;
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
                      const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes,
                      const glm::vec3& viewPosition,
                      const glm::mat4& viewMatrix);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::CommandPool m_commandPool;

    vk::raii::Buffer m_tlasInstanceBuffer = nullptr;
    vk::raii::DeviceMemory m_tlasInstanceBufferMemory = nullptr;

    vk::raii::Buffer m_tlasBuffer = nullptr;
    vk::raii::DeviceMemory m_tlasBufferMemory = nullptr;

    vk::raii::AccelerationStructureKHR m_tlas = nullptr;
    vk::WriteDescriptorSetAccelerationStructureKHR m_tlasInfo{};

    std::shared_ptr<UniformBuffer> m_cameraUniformRT;

    std::shared_ptr<DescriptorSet> m_rayTracingDescriptorSet;

    vk::raii::Buffer m_mergedVertexBuffer = nullptr;
    vk::raii::DeviceMemory m_mergedVertexBufferMemory = nullptr;

    vk::raii::Buffer m_mergedIndexBuffer = nullptr;
    vk::raii::DeviceMemory m_mergedIndexBufferMemory = nullptr;

    vk::raii::Buffer m_meshInfoBuffer = nullptr;
    vk::raii::DeviceMemory m_meshInfoBufferMemory = nullptr;

    vk::DescriptorBufferInfo m_vertexBufferInfo = { nullptr, 0, vk::WholeSize };
    vk::DescriptorBufferInfo m_indexBufferInfo = { nullptr, 0, vk::WholeSize };
    vk::DescriptorBufferInfo m_meshInfoInfo = { nullptr, 0, vk::WholeSize };

    std::vector<vk::DescriptorImageInfo> m_textureImageInfos;

    std::shared_ptr<UniformBuffer> m_cloudUniform;

    float m_speed = 1.0f;

    void createTLAS(const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                    const std::shared_ptr<Cloud>& cloud,
                    const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes);

    [[nodiscard]] uint32_t createTLASInstanceBuffer(const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                                    const std::shared_ptr<Cloud>& cloud,
                                                    const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes);

    void populateInstanceArray(std::vector<vk::AccelerationStructureInstanceKHR>& instances,
                               const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                               const std::shared_ptr<Cloud>& cloud,
                               const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes) const;

    void buildTLAS(vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                   const vk::AccelerationStructureBuildSizesInfoKHR& buildSizesInfo,
                   uint32_t primitiveCount);

    void updateRTSceneInfo(const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    void uploadRTSceneInfoBuffers(const std::vector<Vertex>& mergedVertices,
                                  const std::vector<uint32_t>& mergedIndices,
                                  const std::vector<MeshInfo>& meshInfos);

    void updateRTDescriptorSets(const ImageResource& imageResource,
                                uint32_t currentFrame);

    void updateRTDescriptorSetData(vk::Extent2D extent,
                                   uint32_t currentFrame,
                                   const glm::vec3& viewPosition,
                                   const glm::mat4& viewMatrix);
  };
} // vke

#endif //VULKANPROJECT_RAYTRACER_H