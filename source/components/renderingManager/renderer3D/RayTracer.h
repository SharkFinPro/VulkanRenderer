#ifndef VULKANPROJECT_RAYTRACER_H
#define VULKANPROJECT_RAYTRACER_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace vke {

  class AssetManager;
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
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t textureIndex;
    uint32_t specularIndex;
  };

  class RayTracer {
  public:
    explicit RayTracer(std::shared_ptr<LogicalDevice> logicalDevice,
                       const std::shared_ptr<AssetManager>& assetManager,
                       VkCommandPool commandPool,
                       VkDescriptorPool descriptorPool);

    ~RayTracer();

    void doRayTracing(const RenderInfo* renderInfo,
                      const std::shared_ptr<PipelineManager>& pipelineManager,
                      const std::shared_ptr<LightingManager>& lightingManager,
                      const std::shared_ptr<ImageResource>& imageResource,
                      const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                      const glm::vec3& viewPosition,
                      const glm::mat4& viewMatrix);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkBuffer m_tlasInstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_tlasInstanceBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_tlasBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_tlasBufferMemory = VK_NULL_HANDLE;

    VkAccelerationStructureKHR m_tlas = VK_NULL_HANDLE;
    VkWriteDescriptorSetAccelerationStructureKHR m_tlasInfo{};

    std::shared_ptr<UniformBuffer> m_cameraUniformRT;

    std::shared_ptr<DescriptorSet> m_rayTracingDescriptorSet;

    VkBuffer m_mergedVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_mergedVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_mergedIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_mergedIndexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_meshInfoBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_meshInfoBufferMemory = VK_NULL_HANDLE;

    VkDescriptorBufferInfo m_vertexBufferInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo m_indexBufferInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo m_meshInfoInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };

    std::vector<VkDescriptorImageInfo> m_textureImageInfos;

    void createTLAS(const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    [[nodiscard]] uint32_t createTLASInstanceBuffer(const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    void destroyTLAS();

    void updateRTSceneInfo(const std::vector<std::shared_ptr<RenderObject>>& renderObjects);

    void uploadRTSceneInfoBuffers(const std::vector<Vertex>& mergedVertices,
                                  const std::vector<uint32_t>& mergedIndices,
                                  const std::vector<MeshInfo>& meshInfos);

    void updateRTDescriptorSets(const std::shared_ptr<ImageResource>& imageResource,
                                VkExtent2D extent,
                                uint32_t currentFrame);

    void updateRTDescriptorSetData(VkExtent2D extent,
                                   uint32_t currentFrame,
                                   const glm::vec3& viewPosition,
                                   const glm::mat4& viewMatrix);
  };
} // vke

#endif //VULKANPROJECT_RAYTRACER_H