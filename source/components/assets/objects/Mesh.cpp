#include "Mesh.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../uploadQueue/UploadQueue.h"
#include "../../../utilities/Buffers.h"
#include <glm/vec3.hpp>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace vke {

  namespace {

    constexpr size_t POSITION_BYTES = sizeof(float) * 3;

    void validate(const MeshCreateInfo& createInfo,
                  const bool buildingBlas)
    {
      if (createInfo.vertexStride == 0)
      {
        throw std::invalid_argument("Mesh: vertex stride is zero");
      }

      if (static_cast<size_t>(createInfo.positionOffset) + POSITION_BYTES > createInfo.vertexStride)
      {
        throw std::invalid_argument("Mesh: position does not fit in the vertex stride");
      }

      if (createInfo.vertexData.empty())
      {
        throw std::invalid_argument("Mesh: no vertices");
      }

      if (createInfo.vertexData.size() % createInfo.vertexStride != 0)
      {
        throw std::invalid_argument("Mesh: vertex data is not a multiple of the vertex stride");
      }

      if (createInfo.vertexData.size() / createInfo.vertexStride > UINT32_MAX)
      {
        throw std::invalid_argument("Mesh: too many vertices");
      }

      if (createInfo.indices.empty())
      {
        throw std::invalid_argument("Mesh: no indices");
      }

      if (createInfo.indices.size() % 3 != 0)
      {
        throw std::invalid_argument("Mesh: index count is not a multiple of three");
      }

      if (createInfo.indices.size() > UINT32_MAX)
      {
        throw std::invalid_argument("Mesh: too many indices");
      }

      const size_t vertexCount = createInfo.vertexData.size() / createInfo.vertexStride;

      for (const uint32_t index : createInfo.indices)
      {
        if (index >= vertexCount)
        {
          throw std::invalid_argument("Mesh: index out of range");
        }
      }

      if (buildingBlas && (createInfo.vertexStride % 4 != 0 || createInfo.positionOffset % 4 != 0))
      {
        throw std::invalid_argument("Mesh: a BLAS needs a vertex stride and position offset that are multiples of four");
      }
    }

  } // namespace

  Mesh::Mesh(const std::shared_ptr<LogicalDevice>& logicalDevice,
             const std::shared_ptr<UploadQueue>& uploadQueue,
             const MeshCreateInfo& createInfo)
    : m_logicalDevice(logicalDevice),
      m_state(std::make_shared<UploadState>()),
      m_vertexStride(createInfo.vertexStride)
  {
    const bool rayTracing = logicalDevice->getPhysicalDevice()->supportsRayTracing();
    const bool buildBlas = createInfo.buildBLAS && rayTracing;

    // Against the request, not the hardware, so the same input is accepted or rejected on every device
    validate(createInfo, createInfo.buildBLAS);

    m_vertexCount = static_cast<uint32_t>(createInfo.vertexData.size() / createInfo.vertexStride);
    m_indexCount = static_cast<uint32_t>(createInfo.indices.size());

    computeBounds(createInfo);

    const vk::DeviceSize vertexBytes = createInfo.vertexData.size();
    const vk::DeviceSize indexBytes = createInfo.indices.size_bytes();

    const vk::BufferUsageFlags rayTracingUsage = rayTracing
      ? vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
      : vk::BufferUsageFlags{};

    Buffers::createBuffer(
      logicalDevice,
      vertexBytes,
      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | rayTracingUsage,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_resources.vertexBuffer,
      m_resources.vertexMemory
    );

    Buffers::createBuffer(
      logicalDevice,
      indexBytes,
      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | rayTracingUsage,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_resources.indexBuffer,
      m_resources.indexMemory
    );

    UploadRequest request = uploadQueue->createRequest(m_state, vertexBytes + indexBytes);

    request.write(0, createInfo.vertexData.data(), vertexBytes);
    request.write(vertexBytes, createInfo.indices.data(), indexBytes);

    request.copies.push_back(UploadCopy {
      .destination = *m_resources.vertexBuffer,
      .sourceOffset = 0,
      .destinationOffset = 0,
      .size = vertexBytes
    });
    request.copies.push_back(UploadCopy {
      .destination = *m_resources.indexBuffer,
      .sourceOffset = vertexBytes,
      .destinationOffset = 0,
      .size = indexBytes
    });

    if (buildBlas)
    {
      createBlas(logicalDevice, createInfo, request);
    }

    uploadQueue->enqueue(std::move(request));
  }

  void Mesh::computeBounds(const MeshCreateInfo& createInfo)
  {
    for (uint32_t i = 0; i < m_vertexCount; ++i)
    {
      glm::vec3 position;
      std::memcpy(&position, createInfo.vertexData.data() + static_cast<size_t>(i) * createInfo.vertexStride + createInfo.positionOffset, POSITION_BYTES);

      m_bounds.expand(position);
    }
  }

  void Mesh::createBlas(const std::shared_ptr<LogicalDevice>& logicalDevice,
                        const MeshCreateInfo& createInfo,
                        UploadRequest& request)
  {
    const vk::DeviceAddress vertexAddress = logicalDevice->getBufferDeviceAddress(*m_resources.vertexBuffer) + createInfo.positionOffset;
    const vk::DeviceAddress indexAddress = logicalDevice->getBufferDeviceAddress(*m_resources.indexBuffer);
    const uint32_t primitiveCount = m_indexCount / 3;

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData {};
    trianglesData.vertexFormat = vk::Format::eR32G32B32Sfloat;
    trianglesData.vertexData.deviceAddress = vertexAddress;
    trianglesData.vertexStride = createInfo.vertexStride;
    trianglesData.maxVertex = m_vertexCount - 1;
    trianglesData.indexType = vk::IndexType::eUint32;
    trianglesData.indexData.deviceAddress = indexAddress;

    vk::AccelerationStructureGeometryKHR geometry {};
    geometry.geometryType = vk::GeometryTypeKHR::eTriangles;
    geometry.geometry.triangles = trianglesData;
    geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {};
    buildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    buildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    buildGeometryInfo.geometryCount = 1;
    buildGeometryInfo.pGeometries = &geometry;

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo {};
    logicalDevice->getAccelerationStructureBuildSizes(buildGeometryInfo, primitiveCount, buildSizesInfo);

    Buffers::createBuffer(
      logicalDevice,
      buildSizesInfo.accelerationStructureSize,
      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_resources.blasBuffer,
      m_resources.blasMemory
    );

    const vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo {
      .buffer = *m_resources.blasBuffer,
      .size = buildSizesInfo.accelerationStructureSize,
      .type = vk::AccelerationStructureTypeKHR::eBottomLevel
    };

    m_resources.blas = logicalDevice->createAccelerationStructure(accelerationStructureCreateInfo);

    UploadBlasBuild build;
    build.vertexAddress = vertexAddress;
    build.vertexStride = createInfo.vertexStride;
    build.maxVertex = m_vertexCount - 1;
    build.indexAddress = indexAddress;
    build.primitiveCount = primitiveCount;
    build.destination = *m_resources.blas;

    Buffers::createBuffer(
      logicalDevice,
      buildSizesInfo.buildScratchSize,
      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      build.scratchBuffer,
      build.scratchMemory
    );

    build.scratchAddress = logicalDevice->getBufferDeviceAddress(*build.scratchBuffer);

    request.blas = std::move(build);
  }

  Mesh::~Mesh()
  {
    // Frames in flight may still draw this mesh.
    m_logicalDevice->retireHandles(std::move(m_resources));
  }

  bool Mesh::isReady() const
  {
    return m_state->ready;
  }

  void Mesh::draw(const std::shared_ptr<CommandBuffer>& commandBuffer,
                  const uint32_t firstInstance,
                  const uint32_t instanceCount) const
  {
    if (!m_state->ready)
    {
      return;
    }

    constexpr vk::DeviceSize offsets = 0;
    commandBuffer->bindVertexBuffers(0, { *m_resources.vertexBuffer }, offsets);

    commandBuffer->bindIndexBuffer(*m_resources.indexBuffer, 0, vk::IndexType::eUint32);

    commandBuffer->drawIndexed(m_indexCount, instanceCount, 0, 0, firstInstance);
  }

  const AABB& Mesh::getBounds() const
  {
    return m_bounds;
  }

  uint32_t Mesh::getVertexStride() const
  {
    return m_vertexStride;
  }

  uint32_t Mesh::getVertexCount() const
  {
    return m_vertexCount;
  }

  uint32_t Mesh::getIndexCount() const
  {
    return m_indexCount;
  }

  bool Mesh::hasBLAS() const
  {
    return static_cast<bool>(*m_resources.blas);
  }

  vk::AccelerationStructureKHR Mesh::getBLAS() const
  {
    return *m_resources.blas;
  }

  vk::Buffer Mesh::getVertexBuffer() const
  {
    return *m_resources.vertexBuffer;
  }

  vk::Buffer Mesh::getIndexBuffer() const
  {
    return *m_resources.indexBuffer;
  }

} // namespace vke
