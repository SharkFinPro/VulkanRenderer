#include "Model.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../../utilities/Buffers.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stdexcept>

namespace vke {

  Model::Model(const std::shared_ptr<LogicalDevice>& logicalDevice,
               const vk::CommandPool& commandPool,
               const char* path,
               const glm::vec3 rotation)
  {
    loadModel(path, glm::quat(glm::radians(rotation)));

    createVertexBuffer(logicalDevice, commandPool);
    createIndexBuffer(logicalDevice, commandPool);
    createBLAS(logicalDevice, commandPool);
  }

  Model::Model(const std::shared_ptr<LogicalDevice>& logicalDevice,
               const vk::CommandPool& commandPool,
               const char* path,
               const glm::quat orientation)
  {
    loadModel(path, glm::normalize(orientation));

    createVertexBuffer(logicalDevice, commandPool);
    createIndexBuffer(logicalDevice, commandPool);
    createBLAS(logicalDevice, commandPool);
  }

  void Model::loadModel(const char* path,
                        const glm::quat orientation)
  {
    Assimp::Importer importer;
    constexpr auto sceneFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
                                   aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices;
    const aiScene* scene = importer.ReadFile(path, sceneFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      throw std::runtime_error("Assimp Error: " + std::string(importer.GetErrorString()));
    }

    const aiMesh* mesh = scene->mMeshes[0];
    loadVertices(mesh, orientation);
    loadIndices(mesh);
  }

  void Model::loadVertices(const aiMesh* mesh,
                           const glm::quat orientation)
  {
    const auto orientationMatrix = glm::mat4(orientation);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
      Vertex vertex {
        .pos = {
          mesh->mVertices[i].x,
          mesh->mVertices[i].y,
          mesh->mVertices[i].z
        },
        .normal = {
          mesh->mNormals[i].x,
          mesh->mNormals[i].y,
          mesh->mNormals[i].z
        },
        .texCoord = {
          mesh->mTextureCoords[0][i].x,
          mesh->mTextureCoords[0][i].y
        }
      };

      vertex.pos = orientationMatrix * glm::vec4(vertex.pos, 1.0f);
      vertex.normal = orientationMatrix * glm::vec4(vertex.normal, 1.0f);

      m_vertices.push_back(vertex);
    }
  }

  void Model::loadIndices(const aiMesh* mesh)
  {
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
      const aiFace face = mesh->mFaces[i];

      for (unsigned int j = 0; j < face.mNumIndices; j++)
      {
        m_indices.push_back(face.mIndices[j]);
      }
    }
  }

  void Model::createVertexBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const vk::CommandPool& commandPool)
  {
    const vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    Buffers::createBuffer(
      logicalDevice,
      bufferSize,
      vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      stagingBuffer,
      stagingBufferMemory
    );

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [this, bufferSize](void* data) {
      memcpy(data, m_vertices.data(), bufferSize);
    });

    Buffers::createBuffer(
      logicalDevice,
      bufferSize,
      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer |
      (logicalDevice->getPhysicalDevice()->supportsRayTracing() ? vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR : vk::BufferUsageFlags{}),
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_vertexBuffer,
      m_vertexBufferMemory
    );

    Buffers::copyBuffer(
      logicalDevice,
      commandPool,
      logicalDevice->getGraphicsQueue(),
      stagingBuffer,
      m_vertexBuffer,
      bufferSize
    );
  }

  void Model::createIndexBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                const vk::CommandPool& commandPool)
  {
    const vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;

    Buffers::createBuffer(
      logicalDevice,
      bufferSize,
      vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      stagingBuffer,
      stagingBufferMemory
    );

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [this, bufferSize](void* data) {
      memcpy(data, m_indices.data(), bufferSize);
    });

    Buffers::createBuffer(
      logicalDevice,
      bufferSize,
      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer |
      (logicalDevice->getPhysicalDevice()->supportsRayTracing() ? vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR : vk::BufferUsageFlags{}),
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_indexBuffer,
      m_indexBufferMemory
    );

    Buffers::copyBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue(), *stagingBuffer,
                        *m_indexBuffer, bufferSize);
  }

  void Model::bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    const std::vector<vk::DeviceSize> offsets = {0};
    commandBuffer->bindVertexBuffers(0, { m_vertexBuffer }, offsets);

    commandBuffer->bindIndexBuffer(*m_indexBuffer, 0, vk::IndexType::eUint32);
  }

  void Model::createBLAS(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const vk::CommandPool& commandPool)
  {
    if (!logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{};
    vk::AccelerationStructureGeometryKHR geometry{};
    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};

    createCoreBLASData(logicalDevice, trianglesData, geometry, buildGeometryInfo);

    const auto primitiveCount = static_cast<uint32_t>(m_indices.size() / 3);

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo{};

    logicalDevice->getAccelerationStructureBuildSizes(buildGeometryInfo, primitiveCount, buildSizesInfo);

    Buffers::createBuffer(
      logicalDevice,
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

    m_blas = logicalDevice->createAccelerationStructure(accelerationStructureCreateInfo);

    populateBLAS(logicalDevice, commandPool, buildGeometryInfo, buildSizesInfo, primitiveCount);
  }

  void Model::createCoreBLASData(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 vk::AccelerationStructureGeometryTrianglesDataKHR& trianglesData,
                                 vk::AccelerationStructureGeometryKHR& geometry,
                                 vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo) const
  {
    trianglesData = {
      .vertexFormat = vk::Format::eR32G32B32Sfloat,
      .vertexData = logicalDevice->getBufferDeviceAddress(*m_vertexBuffer),
      .vertexStride = sizeof(Vertex),
      .maxVertex = static_cast<uint32_t>(m_vertices.size() - 1),
      .indexType = vk::IndexType::eUint32,
      .indexData = logicalDevice->getBufferDeviceAddress(*m_indexBuffer)
    };

    geometry = {
      .geometryType = vk::GeometryTypeKHR::eTriangles,
      .geometry = trianglesData,
      .flags = vk::GeometryFlagBitsKHR::eOpaque
    };

    buildGeometryInfo = {
      .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
      .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
      .geometryCount = 1,
      .pGeometries = &geometry
    };
  }

  void Model::populateBLAS(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const vk::CommandPool& commandPool,
                           vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                           const vk::AccelerationStructureBuildSizesInfoKHR& buildSizesInfo,
                           const uint32_t primitiveCount) const
  {
    vk::raii::Buffer scratchBuffer = nullptr;
    vk::raii::DeviceMemory scratchBufferMemory = nullptr;

    Buffers::createBuffer(
      logicalDevice,
      buildSizesInfo.buildScratchSize,
      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      scratchBuffer,
      scratchBufferMemory
    );

    buildGeometryInfo.dstAccelerationStructure = *m_blas;
    buildGeometryInfo.scratchData.deviceAddress = logicalDevice->getBufferDeviceAddress(*scratchBuffer);

    const vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo {
      .primitiveCount = primitiveCount,
      .primitiveOffset = 0,
      .firstVertex = 0,
      .transformOffset = 0
    };

    const auto commandBuffer = SingleUseCommandBuffer(logicalDevice, commandPool, logicalDevice->getGraphicsQueue());

    commandBuffer.record([&commandBuffer, buildGeometryInfo, buildRangeInfo] {
      commandBuffer.buildAccelerationStructure(buildGeometryInfo, &buildRangeInfo);
    });
  }

  void Model::draw(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    bind(commandBuffer);

    commandBuffer->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
  }

  vk::AccelerationStructureKHR Model::getBLAS() const
  {
    return *m_blas;
  }

  const std::vector<Vertex>& Model::getVertices() const
  {
    return m_vertices;
  }

  const std::vector<uint32_t>& Model::getIndices() const
  {
    return m_indices;
  }
} // namespace vke