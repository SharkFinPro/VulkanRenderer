#include "Cloud.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <cstring>

constexpr uint32_t PRIMITIVE_COUNT = 1;
constexpr vk::DeviceSize AABB_BUFFER_SIZE = sizeof(vk::AabbPositionsKHR);

namespace vke {

  Cloud::Cloud(std::shared_ptr<LogicalDevice> logicalDevice,
               const vk::CommandPool& commandPool)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createAABBBuffer(commandPool);

    createBLAS(commandPool);
  }

  Cloud::~Cloud()
  {
    m_logicalDevice->waitIdle();
  }

  vk::AccelerationStructureKHR Cloud::getBLAS() const
  {
    return *m_blas;
  }

  CloudUniform Cloud::getUniformData() const
  {
    return m_uniformData;
  }

  float Cloud::getFrequency() const
  {
    return m_uniformData.frequency;
  }

  float Cloud::getAmplitude() const
  {
    return m_uniformData.amplitude;
  }

  float Cloud::getDensity() const
  {
    return m_uniformData.density;
  }

  float Cloud::getYScale() const
  {
    return m_uniformData.yScale;
  }

  float Cloud::getTime() const
  {
    return m_uniformData.time;
  }

  glm::vec3 Cloud::getTranslation() const
  {
    return m_translation;
  }

  glm::vec3 Cloud::getScale() const
  {
    return m_scale;
  }

  void Cloud::setFrequency(const float frequency)
  {
    m_uniformData.frequency = frequency;
  }

  void Cloud::setAmplitude(const float amplitude)
  {
    m_uniformData.amplitude = amplitude;
  }

  void Cloud::setDensity(const float density)
  {
    m_uniformData.density = density;
  }

  void Cloud::setYScale(const float yScale)
  {
    m_uniformData.yScale = yScale;
  }

  void Cloud::setTime(const float time)
  {
    m_uniformData.time = time;
  }

  void Cloud::setTranslation(const glm::vec3 translation)
  {
    m_translation = translation;
  }

  void Cloud::setScale(const glm::vec3 scale)
  {
    m_scale = scale;
  }

  void Cloud::createAABBBuffer(const vk::CommandPool& commandPool)
  {
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    Buffers::createBuffer(
      m_logicalDevice,
      AABB_BUFFER_SIZE,
      vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      stagingBuffer,
      stagingBufferMemory
    );

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [this](void* data) {
      memcpy(data, &m_aabbPositions, AABB_BUFFER_SIZE);
    });

    Buffers::createBuffer(
      m_logicalDevice,
      AABB_BUFFER_SIZE,
      vk::BufferUsageFlagBits::eTransferDst |
      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
      vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_aabbBuffer,
      m_aabbBufferMemory
    );

    Buffers::copyBuffer(
      m_logicalDevice,
      commandPool,
      m_logicalDevice->getGraphicsQueue(),
      stagingBuffer,
      m_aabbBuffer,
      AABB_BUFFER_SIZE
    );
  }

  void Cloud::createBLAS(const vk::CommandPool& commandPool)
  {
    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }

    vk::AccelerationStructureGeometryAabbsDataKHR aabbsData{};
    vk::AccelerationStructureGeometryKHR geometry{};
    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};

    createCoreBLASData(aabbsData, geometry, buildGeometryInfo);

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo{};

    m_logicalDevice->getAccelerationStructureBuildSizes(buildGeometryInfo, PRIMITIVE_COUNT, buildSizesInfo);

    Buffers::createBuffer(
      m_logicalDevice,
      buildSizesInfo.accelerationStructureSize,
      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_blasBuffer,
      m_blasBufferMemory
    );

    const vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo {
      .buffer = *m_blasBuffer,
      .size = buildSizesInfo.accelerationStructureSize,
      .type = vk::AccelerationStructureTypeKHR::eBottomLevel
    };

    m_blas = m_logicalDevice->createAccelerationStructure(accelerationStructureCreateInfo);

    populateBLAS(commandPool, buildGeometryInfo, buildSizesInfo);
  }

  void Cloud::createCoreBLASData(vk::AccelerationStructureGeometryAabbsDataKHR& aabbsData,
                                 vk::AccelerationStructureGeometryKHR& geometry,
                                 vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo) const
  {
    aabbsData = vk::AccelerationStructureGeometryAabbsDataKHR{
      .data = vk::DeviceOrHostAddressConstKHR{ m_logicalDevice->getBufferDeviceAddress(m_aabbBuffer) },
      .stride = sizeof(vk::AabbPositionsKHR)
    };

    geometry = vk::AccelerationStructureGeometryKHR{
      .geometryType = vk::GeometryTypeKHR::eAabbs,
      .geometry = aabbsData
    };

    buildGeometryInfo = vk::AccelerationStructureBuildGeometryInfoKHR{
      .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
      .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
      .geometryCount = 1,
      .pGeometries = &geometry
    };
  }

  void Cloud::populateBLAS(const vk::CommandPool& commandPool,
                           vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                           const vk::AccelerationStructureBuildSizesInfoKHR& buildSizesInfo) const
  {
    vk::raii::Buffer scratchBuffer = nullptr;
    vk::raii::DeviceMemory scratchBufferMemory = nullptr;

    Buffers::createBuffer(
      m_logicalDevice,
      buildSizesInfo.buildScratchSize,
      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      scratchBuffer,
      scratchBufferMemory
    );

    buildGeometryInfo.dstAccelerationStructure = *m_blas;
    buildGeometryInfo.scratchData.deviceAddress = m_logicalDevice->getBufferDeviceAddress(*scratchBuffer);

    constexpr vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo {
      .primitiveCount = PRIMITIVE_COUNT,
      .primitiveOffset = 0,
      .firstVertex = 0,
      .transformOffset = 0
    };

    const auto commandBuffer = SingleUseCommandBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue());

    commandBuffer.record([&commandBuffer, buildGeometryInfo, buildRangeInfo] {
      commandBuffer.buildAccelerationStructure(buildGeometryInfo, &buildRangeInfo);
    });
  }
} // vke