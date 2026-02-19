#include "Model.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/implementations/vertexInputs/Vertex.h"
#include "../../../utilities/Buffers.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stdexcept>

namespace vke {

  Model::Model(std::shared_ptr<LogicalDevice> logicalDevice,
               const VkCommandPool& commandPool,
               const char* path,
               const glm::vec3 rotation)
    : m_logicalDevice(std::move(logicalDevice))
  {
    loadModel(path, glm::quat(glm::radians(rotation)));

    createVertexBuffer(commandPool);
    createIndexBuffer(commandPool);
    createBLAS(commandPool);
  }

  Model::Model(std::shared_ptr<LogicalDevice> logicalDevice,
               const VkCommandPool& commandPool,
               const char* path,
               const glm::quat orientation)
    : m_logicalDevice(std::move(logicalDevice))
  {
    loadModel(path, glm::normalize(orientation));

    createVertexBuffer(commandPool);
    createIndexBuffer(commandPool);
    createBLAS(commandPool);
  }

  Model::~Model()
  {
    Buffers::destroyBuffer(m_logicalDevice, m_indexBuffer, m_indexBufferMemory);

    Buffers::destroyBuffer(m_logicalDevice, m_vertexBuffer, m_vertexBufferMemory);
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

  void Model::createVertexBuffer(const VkCommandPool& commandPool)
  {
    const VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Buffers::createBuffer(m_logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer, stagingBufferMemory);

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [this, bufferSize](void* data) {
      memcpy(data, m_vertices.data(), bufferSize);
    });

    Buffers::createBuffer(m_logicalDevice, bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

    Buffers::copyBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue(), stagingBuffer,
                        m_vertexBuffer, bufferSize);

    Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);
  }

  void Model::createIndexBuffer(const VkCommandPool& commandPool)
  {
    const VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    Buffers::createBuffer(m_logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer, stagingBufferMemory);

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [this, bufferSize](void* data) {
      memcpy(data, m_indices.data(), bufferSize);
    });

    Buffers::createBuffer(m_logicalDevice, bufferSize,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

    Buffers::copyBuffer(m_logicalDevice, commandPool, m_logicalDevice->getGraphicsQueue(), stagingBuffer,
                        m_indexBuffer, bufferSize);

    Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);
  }

  void Model::bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    constexpr VkDeviceSize offsets[] = {0};
    commandBuffer->bindVertexBuffers(0, 1, &m_vertexBuffer, offsets);

    commandBuffer->bindIndexBuffer(m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  }

  void Model::createBLAS(const VkCommandPool& commandPool)
  {
    VkAccelerationStructureGeometryTrianglesDataKHR trianglesData {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
      .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
      .vertexStride = sizeof(Vertex),
      .maxVertex = static_cast<uint32_t>(m_vertices.size() - 1),
      .indexType = VK_INDEX_TYPE_UINT32,
      .indexData {
        .deviceAddress = m_logicalDevice->getBufferDeviceAddress(m_indexBuffer)
      },
      .transformData = {}
    };

    VkAccelerationStructureGeometryKHR geometry {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
      .geometry = {
        .triangles = trianglesData
      },
      .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .geometryCount = 1,
      .pGeometries = &geometry
    };

    const auto primitiveCount = static_cast<uint32_t>(m_indices.size() / 3);

    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
    };

    m_logicalDevice->getAccelerationStructureBuildSizes(&buildGeometryInfo, &primitiveCount, &buildSizesInfo);

    Buffers::createBuffer(
      m_logicalDevice,
      buildSizesInfo.accelerationStructureSize,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_blasBuffer,
      m_blasBufferMemory
    );

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .buffer = m_blasBuffer,
      .size = buildSizesInfo.accelerationStructureSize,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };

    m_logicalDevice->createAccelerationStructure(accelerationStructureCreateInfo, &m_blas);
  }

  void Model::draw(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    bind(commandBuffer);

    commandBuffer->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
  }

} // namespace vke