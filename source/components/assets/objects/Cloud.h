#ifndef VULKANPROJECT_CLOUD_H
#define VULKANPROJECT_CLOUD_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class LogicalDevice;

  struct CloudUniform {
    float frequency = 20.0f;
    float amplitude = 3.0f;
    float density = 0.15f;
    float yScale = 0.15f;
    float time = 0.0f;
  };

  class Cloud {
  public:
    Cloud(std::shared_ptr<LogicalDevice> logicalDevice,
          const VkCommandPool& commandPool);

    ~Cloud();

    [[nodiscard]] VkAccelerationStructureKHR getBLAS() const;

    [[nodiscard]] CloudUniform getUniformData() const;

    [[nodiscard]] float getFrequency() const;
    [[nodiscard]] float getAmplitude() const;
    [[nodiscard]] float getDensity() const;
    [[nodiscard]] float getYScale() const;
    [[nodiscard]] float getTime() const;

    void setFrequency(float frequency);
    void setAmplitude(float amplitude);
    void setDensity(float density);
    void setYScale(float yScale);
    void setTime(float time);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkBuffer m_blasBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_blasBufferMemory = VK_NULL_HANDLE;
    VkAccelerationStructureKHR m_blas = VK_NULL_HANDLE;

    VkAabbPositionsKHR m_aabbPositions {
      .minX = -1.0f, .minY = -1.0f, .minZ = -1.0f,
      .maxX =  1.0f, .maxY = 1.0f, .maxZ =  1.0f
    };

    VkBuffer m_aabbBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_aabbBufferMemory = VK_NULL_HANDLE;

    CloudUniform m_uniformData;

    void createAABBBuffer(const VkCommandPool& commandPool);

    void createBLAS(const VkCommandPool& commandPool);

    void createCoreBLASData(VkAccelerationStructureGeometryAabbsDataKHR& aabbsData,
                            VkAccelerationStructureGeometryKHR& geometry,
                            VkAccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo) const;

    void populateBLAS(const VkCommandPool& commandPool,
                      VkAccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                      const VkAccelerationStructureBuildSizesInfoKHR& buildSizesInfo) const;
  };
} // vke

#endif //VULKANPROJECT_CLOUD_H