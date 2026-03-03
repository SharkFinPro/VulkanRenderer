#include "RayTracer.h"
#include "../ImageResource.h"
#include "../../assets/AssetManager.h"
#include "../../assets/objects/Cloud.h"
#include "../../assets/objects/Model.h"
#include "../../assets/objects/RenderObject.h"
#include "../../assets/textures/Texture.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../lighting/LightingManager.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../pipelines/GraphicsPipeline.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/implementations/vertexInputs/Vertex.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include "../../../utilities/Buffers.h"

namespace vke {

  class Texture;

  struct CameraUniformRT {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    glm::vec3 viewPosition;
  };

  RayTracer::RayTracer(std::shared_ptr<LogicalDevice> logicalDevice,
                       const std::shared_ptr<AssetManager>& assetManager,
                       VkCommandPool commandPool,
                       VkDescriptorPool descriptorPool)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(commandPool)
  {
    std::vector<uint32_t> maxTextures;
    for (uint32_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); ++i)
    {
      maxTextures.push_back(256);
    }

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
      .descriptorSetCount = static_cast<uint32_t>(maxTextures.size()),
      .pDescriptorCounts = maxTextures.data()
    };

    m_rayTracingDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool,
      assetManager->getRayTracingDescriptorSetLayout(), &variableCountInfo);

    m_cameraUniformRT = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniformRT));

    m_cloudUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CloudUniform));
  }

  RayTracer::~RayTracer()
  {
    destroyTLAS();
  }

  void RayTracer::doRayTracing(const RenderInfo* renderInfo,
                               const std::shared_ptr<PipelineManager>& pipelineManager,
                               const std::shared_ptr<LightingManager>& lightingManager,
                               const std::shared_ptr<ImageResource>& imageResource,
                               const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                               const std::shared_ptr<Cloud>& cloud,
                               const glm::vec3& viewPosition,
                               const glm::mat4& viewMatrix)
  {
    createTLAS(renderObjects, cloud);

    updateRTSceneInfo(renderObjects);

    updateRTDescriptorSetData(renderInfo->extent, renderInfo->currentFrame, viewPosition, viewMatrix);

    if (cloud)
    {
      const auto cloudUBO = cloud->getUniformData();
      m_cloudUniform->update(renderInfo->currentFrame, &cloudUBO);
    }

    updateRTDescriptorSets(imageResource, renderInfo->extent, renderInfo->currentFrame);

    pipelineManager->bindRayTracingPipelineDescriptorSet(
      renderInfo->commandBuffer,
      m_rayTracingDescriptorSet->getDescriptorSet(renderInfo->currentFrame),
      0
    );

    pipelineManager->bindRayTracingPipelineDescriptorSet(
      renderInfo->commandBuffer,
      lightingManager->getLightingDescriptorSet()->getDescriptorSet(renderInfo->currentFrame),
      1
    );

    pipelineManager->doRayTracing(renderInfo->commandBuffer, renderInfo->extent);
  }

  void RayTracer::createTLAS(const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                             const std::shared_ptr<Cloud>& cloud)
  {
    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }

    destroyTLAS();

    const auto primitiveCount = createTLASInstanceBuffer(renderObjects, cloud);

    VkAccelerationStructureGeometryInstancesDataKHR instancesData {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
      .arrayOfPointers = VK_FALSE,
      .data = {
        .deviceAddress = m_logicalDevice->getBufferDeviceAddress(m_tlasInstanceBuffer)
      }
    };

    VkAccelerationStructureGeometryKHR geometry {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
      .geometry = {
        .instances = instancesData
      }
    };

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .geometryCount = 1,
      .pGeometries = &geometry
    };

    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    m_logicalDevice->getAccelerationStructureBuildSizes(&buildGeometryInfo, &primitiveCount, &buildSizesInfo);

    Buffers::createBuffer(
      m_logicalDevice,
      buildSizesInfo.accelerationStructureSize,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_tlasBuffer,
      m_tlasBufferMemory
    );

    buildTLAS(buildGeometryInfo, buildSizesInfo, primitiveCount);
  }

  uint32_t RayTracer::createTLASInstanceBuffer(const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                               const std::shared_ptr<Cloud>& cloud)
  {
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.reserve(renderObjects.size());

    populateInstanceArray(instances, renderObjects, cloud);

    const VkDeviceSize instancesBufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    Buffers::createBuffer(
      m_logicalDevice,
      instancesBufferSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      stagingBuffer,
      stagingBufferMemory
    );

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [instances, instancesBufferSize](void* data) {
      memcpy(data, instances.data(), instancesBufferSize);
    });

    Buffers::createBuffer(
      m_logicalDevice,
      instancesBufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_tlasInstanceBuffer,
      m_tlasInstanceBufferMemory
    );

    Buffers::copyBuffer(
      m_logicalDevice,
      m_commandPool,
      m_logicalDevice->getGraphicsQueue(),
      stagingBuffer,
      m_tlasInstanceBuffer,
      instancesBufferSize
    );

    Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingBufferMemory);

    return static_cast<uint32_t>(instances.size());
  }

  void RayTracer::populateInstanceArray(std::vector<VkAccelerationStructureInstanceKHR>& instances,
                                        const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                        const std::shared_ptr<Cloud>& cloud) const
  {
    for (const auto& renderObject : renderObjects)
    {
      const glm::mat4 modelMatrix = glm::transpose(renderObject->getModelMatrix());

      VkTransformMatrixKHR transformMatrix;
      memcpy(&transformMatrix, &modelMatrix, sizeof(VkTransformMatrixKHR));

      const VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = renderObject->getModel()->getBLAS()
      };

      const VkAccelerationStructureInstanceKHR instance {
        .transform = transformMatrix,
        .instanceCustomIndex = static_cast<uint32_t>(instances.size()),
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = 0,
        .accelerationStructureReference = m_logicalDevice->getAccelerationStructureDeviceAddress(&accelerationStructureDeviceAddressInfo)
      };

      instances.push_back(instance);
    }

    if (cloud)
    {
      const VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = cloud->getBLAS()
      };

      const glm::mat4 modelTransform = glm::scale(glm::translate(glm::mat4(1.0f), cloud->getTranslation()), cloud->getScale());

      const glm::mat4 modelMatrix = glm::transpose(modelTransform);
      VkTransformMatrixKHR transformMatrix;
      memcpy(&transformMatrix, &modelMatrix, sizeof(VkTransformMatrixKHR));

      const VkAccelerationStructureInstanceKHR instance {
        .transform = transformMatrix,
        .instanceCustomIndex = static_cast<uint32_t>(instances.size()),
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 1,
        .flags = 0,
        .accelerationStructureReference = m_logicalDevice->getAccelerationStructureDeviceAddress(&accelerationStructureDeviceAddressInfo)
      };

      instances.push_back(instance);
    }
  }

  void RayTracer::buildTLAS(VkAccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                            const VkAccelerationStructureBuildSizesInfoKHR& buildSizesInfo,
                            const uint32_t primitiveCount)
  {
    const VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .buffer = m_tlasBuffer,
      .size = buildSizesInfo.accelerationStructureSize,
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
    };

    m_logicalDevice->createAccelerationStructure(accelerationStructureCreateInfo, &m_tlas);

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

    buildGeometryInfo.dstAccelerationStructure = m_tlas;
    buildGeometryInfo.scratchData.deviceAddress = m_logicalDevice->getBufferDeviceAddress(scratchBuffer);

    const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo {
      .primitiveCount = primitiveCount,
      .primitiveOffset = 0,
      .firstVertex = 0,
      .transformOffset = 0
    };

    const auto commandBuffer = SingleUseCommandBuffer(m_logicalDevice, m_commandPool, m_logicalDevice->getGraphicsQueue());

    commandBuffer.record([commandBuffer, buildGeometryInfo, buildRangeInfo] {
      commandBuffer.buildAccelerationStructure(buildGeometryInfo, &buildRangeInfo);
    });

    Buffers::destroyBuffer(m_logicalDevice, scratchBuffer, scratchBufferMemory);

    m_tlasInfo = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
      .accelerationStructureCount = 1,
      .pAccelerationStructures = &m_tlas
    };
  }

  void RayTracer::destroyTLAS()
  {
    m_logicalDevice->destroyAccelerationStructureKHR(m_tlas);

    Buffers::destroyBuffer(m_logicalDevice, m_tlasBuffer, m_tlasBufferMemory);
    Buffers::destroyBuffer(m_logicalDevice, m_tlasInstanceBuffer, m_tlasInstanceBufferMemory);

    Buffers::destroyBuffer(m_logicalDevice, m_mergedVertexBuffer, m_mergedVertexBufferMemory);
    Buffers::destroyBuffer(m_logicalDevice, m_mergedIndexBuffer, m_mergedIndexBufferMemory);

    Buffers::destroyBuffer(m_logicalDevice, m_meshInfoBuffer, m_meshInfoBufferMemory);
  }

  void RayTracer::updateRTSceneInfo(const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
  {
    std::vector<Vertex> mergedVertices;
    std::vector<uint32_t> mergedIndices;
    std::vector<MeshInfo> meshInfos;
    m_textureImageInfos.clear();

    std::unordered_map<std::shared_ptr<Texture>, uint32_t> textureIndices;

    for (const auto& renderObject : renderObjects)
    {
      const auto& model = renderObject->getModel();

      auto texture = renderObject->getTexture();
      uint32_t textureIndex = 0;
      if (textureIndices.contains(texture))
      {
        textureIndex = textureIndices.at(texture);
      }
      else
      {
        textureIndex = static_cast<uint32_t>(textureIndices.size());

        textureIndices.emplace(texture, textureIndex);

        m_textureImageInfos.push_back(texture->getImageInfo());
      }

      auto specularMap = renderObject->getSpecularMap();
      uint32_t specularIndex = 0;
      if (textureIndices.contains(specularMap))
      {
        specularIndex = textureIndices.at(specularMap);
      }
      else
      {
        specularIndex = static_cast<uint32_t>(textureIndices.size());

        textureIndices.emplace(specularMap, specularIndex);

        m_textureImageInfos.push_back(specularMap->getImageInfo());
      }

      meshInfos.push_back({
        .vertexOffset = static_cast<uint32_t>(mergedVertices.size()),
        .indexOffset = static_cast<uint32_t>(mergedIndices.size()),
        .textureIndex = textureIndex,
        .specularIndex = specularIndex,
        .reflectivity = renderObject->getReflectivity(),
        .refractivity = renderObject->getRefractivity(),
        .indexOfRefraction = renderObject->getIndexOfRefraction()
      });

      const auto& vertices = model->getVertices();
      const auto& indices = model->getIndices();

      mergedVertices.insert(mergedVertices.end(), vertices.begin(), vertices.end());
      mergedIndices.insert(mergedIndices.end(), indices.begin(), indices.end());
    }

    uploadRTSceneInfoBuffers(mergedVertices, mergedIndices, meshInfos);
  }

  void RayTracer::uploadRTSceneInfoBuffers(const std::vector<Vertex>& mergedVertices,
                                           const std::vector<uint32_t>& mergedIndices,
                                           const std::vector<MeshInfo>& meshInfos)
  {
    auto uploadBuffer = [&]<typename T>(const std::vector<T>& data,
                                        VkBuffer& outBuffer,
                                        VkDeviceMemory& outMemory)
    {
      const VkDeviceSize size = data.size() * sizeof(T);

      VkBuffer stagingBuffer;
      VkDeviceMemory stagingMemory;

      Buffers::createBuffer(
        m_logicalDevice,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory
      );

      m_logicalDevice->doMappedMemoryOperation(stagingMemory, [&data, size](void* ptr) {
        memcpy(ptr, data.data(), size);
      });

      Buffers::createBuffer(
        m_logicalDevice,
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        outBuffer,
        outMemory
      );

      Buffers::copyBuffer(
        m_logicalDevice,
        m_commandPool,
        m_logicalDevice->getGraphicsQueue(),
        stagingBuffer,
        outBuffer,
        size
      );

      Buffers::destroyBuffer(m_logicalDevice, stagingBuffer, stagingMemory);
    };

    uploadBuffer(mergedVertices, m_mergedVertexBuffer, m_mergedVertexBufferMemory);
    uploadBuffer(mergedIndices, m_mergedIndexBuffer, m_mergedIndexBufferMemory);
    uploadBuffer(meshInfos, m_meshInfoBuffer, m_meshInfoBufferMemory);
  }

  void RayTracer::updateRTDescriptorSets(const std::shared_ptr<ImageResource>& imageResource,
                                         const VkExtent2D extent,
                                         const uint32_t currentFrame)
  {
    m_rayTracingDescriptorSet->updateDescriptorSets([this, imageResource, currentFrame](VkDescriptorSet descriptorSet, [[maybe_unused]] const size_t frame)
    {
      auto storageBuffer = [&](const uint32_t binding, const VkDescriptorBufferInfo* info) {
        return VkWriteDescriptorSet {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptorSet,
          .dstBinding = binding,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo = info
        };
      };

      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .pNext = &m_tlasInfo,
          .dstSet = descriptorSet,
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
        },
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptorSet,
          .dstBinding = 1,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = &imageResource->getDescriptorImageInfo()
        },
        m_cameraUniformRT->getDescriptorSet(2, descriptorSet, currentFrame),
        storageBuffer(3, &m_vertexBufferInfo),
        storageBuffer(4, &m_indexBufferInfo),
        storageBuffer(5, &m_meshInfoInfo),
        m_cloudUniform->getDescriptorSet(6, descriptorSet, currentFrame),
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptorSet,
          .dstBinding = 7,
          .descriptorCount = static_cast<uint32_t>(m_textureImageInfos.size()),
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = m_textureImageInfos.data()
        },
      }};

      return descriptorWrites;
    });
  }

  void RayTracer::updateRTDescriptorSetData(const VkExtent2D extent,
                                            const uint32_t currentFrame,
                                            const glm::vec3& viewPosition,
                                            const glm::mat4& viewMatrix)
  {
    auto projectionMatrix = glm::perspective(
      glm::radians(45.0f),
      static_cast<float>(extent.width) / static_cast<float>(extent.height),
      0.1f,
      1000.0f
    );

    projectionMatrix[1][1] *= -1;

    const CameraUniformRT cameraUBORT {
      .viewInverse = glm::inverse(viewMatrix),
      .projInverse = glm::inverse(projectionMatrix),
      .viewPosition = viewPosition
    };

    m_cameraUniformRT->update(currentFrame, &cameraUBORT);

    m_vertexBufferInfo.buffer = m_mergedVertexBuffer;
    m_indexBufferInfo.buffer = m_mergedIndexBuffer;
    m_meshInfoInfo.buffer = m_meshInfoBuffer;
  }

} // vke