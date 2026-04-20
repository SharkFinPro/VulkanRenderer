#ifndef VULKANPROJECT_PROCEDURALVOLUME_H
#define VULKANPROJECT_PROCEDURALVOLUME_H

#include <vulkan/vulkan_raii.hpp>
#include <glm/vec3.hpp>

namespace vke {

  class LogicalDevice;

  class ProceduralVolume {
  public:
    ProceduralVolume(std::shared_ptr<LogicalDevice> logicalDevice,
                     const vk::CommandPool& commandPool,
                     vk::AabbPositionsKHR aabbPositions = { -1.f, -1.f, -1.f, 1.f, 1.f, 1.f },
                     glm::vec3 translation = glm::vec3(0.f),
                     glm::vec3 scale = glm::vec3(1.f));

    ~ProceduralVolume();

    [[nodiscard]] vk::AccelerationStructureKHR getBLAS() const;

    [[nodiscard]] glm::vec3 getTranslation() const;
    [[nodiscard]] glm::vec3 getScale() const;

    void setTranslation(glm::vec3 translation);
    void setScale(glm::vec3 scale);

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::Buffer m_blasBuffer = nullptr;
    vk::raii::DeviceMemory m_blasBufferMemory = nullptr;
    vk::raii::AccelerationStructureKHR m_blas = nullptr;

    vk::AabbPositionsKHR m_aabbPositions;

    vk::raii::Buffer m_aabbBuffer = nullptr;
    vk::raii::DeviceMemory m_aabbBufferMemory = nullptr;

    glm::vec3 m_translation;
    glm::vec3 m_scale;

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

#endif //VULKANPROJECT_PROCEDURALVOLUME_H
