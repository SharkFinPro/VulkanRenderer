#include "Cloud.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <cstring>

constexpr uint32_t PRIMITIVE_COUNT = 1;
constexpr VkDeviceSize AABB_BUFFER_SIZE = sizeof(VkAabbPositionsKHR);

namespace vke {

  Cloud::Cloud(std::shared_ptr<LogicalDevice> logicalDevice,
               const VkCommandPool& commandPool)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createAABBBuffer(commandPool);

    createBLAS(commandPool);
  }

  Cloud::~Cloud()
  {
    m_logicalDevice->destroyAccelerationStructureKHR(m_blas);

    Buffers::destroyBuffer(m_logicalDevice, m_blasBuffer, m_blasBufferMemory);
    Buffers::destroyBuffer(m_logicalDevice, m_aabbBuffer, m_aabbBufferMemory);
  }

  VkAccelerationStructureKHR Cloud::getBLAS() const
  {
    return m_blas;
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

  void Cloud::createAABBBuffer(const VkCommandPool& commandPool)
  {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Buffers::createBuffer(
      m_logicalDevice,
      AABB_BUFFER_SIZE,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer,
      stagingBufferMemory
    );

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [this](void* data) {
      memcpy(data, &m_aabbPositions, AABB_BUFFER_SIZE);
    });

    Buffers::createBuffer(
      m_logicalDevice,
      AABB_BUFFER_SIZE,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT |
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
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

    Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);
  }

  void Cloud::createBLAS(const VkCommandPool& commandPool)
  {
    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }

    VkAccelerationStructureGeometryAabbsDataKHR aabbsData{};
    VkAccelerationStructureGeometryKHR geometry{};
    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};

    createCoreBLASData(aabbsData, geometry, buildGeometryInfo);

    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
    };

    m_logicalDevice->getAccelerationStructureBuildSizes(&buildGeometryInfo, &PRIMITIVE_COUNT, &buildSizesInfo);

    Buffers::createBuffer(
      m_logicalDevice,
      buildSizesInfo.accelerationStructureSize,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_blasBuffer,
      m_blasBufferMemory
    );

    const VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .buffer = m_blasBuffer,
      .size = buildSizesInfo.accelerationStructureSize,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };

    m_logicalDevice->createAccelerationStructure(accelerationStructureCreateInfo, &m_blas);

    populateBLAS(commandPool, buildGeometryInfo, buildSizesInfo);
  }

  void Cloud::createCoreBLASData(VkAccelerationStructureGeometryAabbsDataKHR& aabbsData,
                                 VkAccelerationStructureGeometryKHR& geometry,
                                 VkAccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo) const
  {
    aabbsData = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
      .data = {
        .deviceAddress = m_logicalDevice->getBufferDeviceAddress(m_aabbBuffer)
      },
      .stride = sizeof(VkAabbPositionsKHR)
    };

    geometry = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .geometryType = VK_GEOMETRY_TYPE_AABBS_KHR,
      .geometry = {
        .aabbs = aabbsData
      }
    };

    buildGeometryInfo = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .geometryCount = 1,
      .pGeometries = &geometry
    };
  }

  void Cloud::populateBLAS(const VkCommandPool& commandPool,
                           VkAccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                           const VkAccelerationStructureBuildSizesInfoKHR& buildSizesInfo) const
  {
    VkBuffer scratchBuffer;
    VkDeviceMemory scratchBufferMemory;

    Buffers::createBuffer(
      m_logicalDevice,
      buildSizesInfo.buildScratchSize,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      scratchBuffer,
      scratchBufferMemory
    );

    buildGeometryInfo.dstAccelerationStructure = m_blas;
    buildGeometryInfo.scratchData.deviceAddress = m_logicalDevice->getBufferDeviceAddress(scratchBuffer);

    constexpr VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo {
      .primitiveCount = PRIMITIVE_COUNT,
      .primitiveOffset = 0,
      .firstVertex = 0,
      .transformOffset = 0
    };

    const auto commandBuffer = SingleUseCommandBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue());

    commandBuffer.record([commandBuffer, buildGeometryInfo, buildRangeInfo] {
      commandBuffer.buildAccelerationStructure(buildGeometryInfo, &buildRangeInfo);
    });

    Buffers::destroyBuffer(m_logicalDevice, scratchBuffer, scratchBufferMemory);
  }
} // vke