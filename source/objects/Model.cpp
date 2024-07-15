#include "Model.h"
#include "../Buffers.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>

#include "../pipeline/Vertex.h"

Model::Model(VkDevice& device, VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue,
             const char* path)
  : device(device), physicalDevice(physicalDevice)
{
  loadModel(path);

  createVertexBuffer(commandPool, graphicsQueue);
  createIndexBuffer(commandPool, graphicsQueue);
}

Model::~Model()
{
  vkDestroyBuffer(device, indexBuffer, nullptr);
  vkFreeMemory(device, indexBufferMemory, nullptr);

  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void Model::loadModel(const char* path)
{
  /* Load model with assimp */
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
                                                              aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);

  // Check for errors
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
  {
    throw std::runtime_error("Assimp Error: " + std::string(importer.GetErrorString()));
  }

  // Read mesh data
  aiMesh *mesh = scene->mMeshes[0];

  for (unsigned int i = 0; i < mesh->mNumVertices; i++)
  {
    Vertex vertex{};

    vertex.pos = {
      mesh->mVertices[i].x,
      mesh->mVertices[i].y,
      mesh->mVertices[i].z
    };

    vertex.normal = {
      mesh->mNormals[i].x,
      mesh->mNormals[i].y,
      mesh->mNormals[i].z
    };

    vertex.texCoord = {
      mesh->mTextureCoords[0][i].x,
      mesh->mTextureCoords[0][i].y
    };

    vertex.color = {1.0f, 1.0f, 1.0f};

    vertices.push_back(vertex);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; i++)
  {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++)
    {
      indices.push_back(face.mIndices[j]);
    }
  }
}

void Model::createVertexBuffer(VkCommandPool& commandPool, VkQueue& graphicsQueue)
{
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  Buffers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t) bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  Buffers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  Buffers::copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, vertexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Model::createIndexBuffer(VkCommandPool& commandPool, VkQueue& graphicsQueue)
{
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  Buffers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t) bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  Buffers::createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

  Buffers::copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, indexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Model::bind(VkCommandBuffer& commandBuffer)
{
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void Model::draw(VkCommandBuffer& commandBuffer)
{
  bind(commandBuffer);

  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}
