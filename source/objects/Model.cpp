#include "Model.h"
#include "../utilities/Buffers.h"
#include "../pipelines/Vertex.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>

Model::Model(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
             const VkCommandPool& commandPool, const char* path, const glm::vec3 rotation)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{
  loadModel(path, glm::quat(glm::radians(rotation)));

  createVertexBuffer(commandPool);
  createIndexBuffer(commandPool);
}

Model::Model(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
             const VkCommandPool& commandPool, const char* path, const glm::quat orientation)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{
  loadModel(path, glm::normalize(orientation));

  createVertexBuffer(commandPool);
  createIndexBuffer(commandPool);
}

Model::~Model()
{
  Buffers::destroyBuffer(logicalDevice, indexBuffer, indexBufferMemory);

  Buffers::destroyBuffer(logicalDevice, vertexBuffer, vertexBufferMemory);
}

void Model::loadModel(const char* path, const glm::quat orientation)
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

    vertices.push_back(vertex);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; i++)
  {
    const aiFace face = mesh->mFaces[i];

    for (unsigned int j = 0; j < face.mNumIndices; j++)
    {
      indices.push_back(face.mIndices[j]);
    }
  }
}

void Model::createVertexBuffer(const VkCommandPool& commandPool)
{
  const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(logicalDevice->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), bufferSize);
  vkUnmapMemory(logicalDevice->getDevice(), stagingBufferMemory);

  Buffers::createBuffer(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  Buffers::copyBuffer(logicalDevice->getDevice(), commandPool, logicalDevice->getGraphicsQueue(), stagingBuffer,
                      vertexBuffer, bufferSize);

  Buffers::destroyBuffer(logicalDevice, stagingBuffer, stagingBufferMemory);
}

void Model::createIndexBuffer(const VkCommandPool& commandPool)
{
  const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  Buffers::createBuffer(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(logicalDevice->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), bufferSize);
  vkUnmapMemory(logicalDevice->getDevice(), stagingBufferMemory);

  Buffers::createBuffer(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), bufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

  Buffers::copyBuffer(logicalDevice->getDevice(), commandPool, logicalDevice->getGraphicsQueue(), stagingBuffer,
                      indexBuffer, bufferSize);

  Buffers::destroyBuffer(logicalDevice, stagingBuffer, stagingBufferMemory);
}

void Model::bind(const VkCommandBuffer& commandBuffer) const
{
  constexpr VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void Model::draw(const VkCommandBuffer& commandBuffer) const
{
  bind(commandBuffer);

  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}
