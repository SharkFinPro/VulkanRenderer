#include "ProceduralVolume.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <cstring>

constexpr uint32_t PRIMITIVE_COUNT = 1;
constexpr vk::DeviceSize AABB_BUFFER_SIZE = sizeof(vk::AabbPositionsKHR);

namespace vke {

  ProceduralVolume::ProceduralVolume(std::shared_ptr<LogicalDevice> logicalDevice,
                                     const vk::CommandPool& commandPool,
                                     vk::AabbPositionsKHR aabbPositions,
                                     glm::vec3 translation,
                                     glm::vec3 scale)
    : m_logicalDevice(std::move(logicalDevice)), m_aabbPositions(aabbPositions), m_translation(translation),
      m_scale(scale)
  {
    createAABBBuffer(commandPool);

    createBLAS(commandPool);
  }

  ProceduralVolume::~ProceduralVolume()
  {
    m_logicalDevice->waitIdle();
  }

  vk::AccelerationStructureKHR ProceduralVolume::getBLAS() const
  {
    return *m_blas;
  }

  glm::vec3 ProceduralVolume::getTranslation() const
  {
    return m_translation;
  }

  glm::vec3 ProceduralVolume::getScale() const
  {
    return m_scale;
  }

  void ProceduralVolume::setTranslation(const glm::vec3 translation)
  {
    m_translation = translation;
  }

  void ProceduralVolume::setScale(const glm::vec3 scale)
  {
    m_scale = scale;
  }

  void ProceduralVolume::createAABBBuffer(const vk::CommandPool& commandPool)
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

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [this](void* data) {
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

  void ProceduralVolume::createBLAS(const vk::CommandPool& commandPool)
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

  void ProceduralVolume::createCoreBLASData(vk::AccelerationStructureGeometryAabbsDataKHR& aabbsData,
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

  void ProceduralVolume::populateBLAS(const vk::CommandPool& commandPool,
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