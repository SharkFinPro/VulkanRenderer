#ifndef VULKANPROJECT_CLOUD_H
#define VULKANPROJECT_CLOUD_H

#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
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
          const vk::CommandPool& commandPool);

    [[nodiscard]] vk::AccelerationStructureKHR getBLAS() const;

    [[nodiscard]] CloudUniform getUniformData() const;

    [[nodiscard]] float getFrequency() const;
    [[nodiscard]] float getAmplitude() const;
    [[nodiscard]] float getDensity() const;
    [[nodiscard]] float getYScale() const;
    [[nodiscard]] float getTime() const;
    [[nodiscard]] glm::vec3 getTranslation() const;
    [[nodiscard]] glm::vec3 getScale() const;

    void setFrequency(float frequency);
    void setAmplitude(float amplitude);
    void setDensity(float density);
    void setYScale(float yScale);
    void setTime(float time);
    void setTranslation(glm::vec3 translation);
    void setScale(glm::vec3 scale);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::Buffer m_blasBuffer = nullptr;
    vk::raii::DeviceMemory m_blasBufferMemory = nullptr;
    vk::raii::AccelerationStructureKHR m_blas = nullptr;

    vk::AabbPositionsKHR m_aabbPositions {
      .minX = -1.0f, .minY = -1.0f, .minZ = -1.0f,
      .maxX =  1.0f, .maxY = 1.0f, .maxZ =  1.0f
    };

    vk::raii::Buffer m_aabbBuffer = nullptr;
    vk::raii::DeviceMemory m_aabbBufferMemory = nullptr;

    CloudUniform m_uniformData;

    glm::vec3 m_translation = glm::vec3(0.0f, 500.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(5500.0f, 400.0f, 5500.0f);

    void createAABBBuffer(const vk::CommandPool& commandPool);

    void createBLAS(const vk::CommandPool& commandPool);

    void createCoreBLASData(vk::AccelerationStructureGeometryAabbsDataKHR& aabbsData,
                            vk::AccelerationStructureGeometryKHR& geometry,
                            vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo) const;

    void populateBLAS(const vk::CommandPool& commandPool,
                      vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                      const vk::AccelerationStructureBuildSizesInfoKHR& buildSizesInfo) const;
  };
} // vke

#endif //VULKANPROJECT_CLOUD_H