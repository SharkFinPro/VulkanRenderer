#include "RayTracer.h"
#include "../ImageResource.h"
#include "../../assets/AssetManager.h"
#include "../../assets/objects/Cloud.h"
#include "../../assets/objects/Model.h"
#include "../../assets/objects/RenderObject.h"
#include "../../assets/objects/SmokeVolume.h"
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
                       const vk::CommandPool commandPool,
                       const vk::DescriptorPool descriptorPool)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(commandPool)
  {
    std::vector<uint32_t> maxTextures;
    for (uint32_t i = 0; i < m_logicalDevice->getMaxFramesInFlight(); ++i)
    {
      maxTextures.push_back(256);
    }

    vk::DescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo {
      .descriptorSetCount = static_cast<uint32_t>(maxTextures.size()),
      .pDescriptorCounts = maxTextures.data()
    };

    m_rayTracingDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool,
      assetManager->getRayTracingDescriptorSetLayout(), &variableCountInfo);

    m_cameraUniformRT = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniformRT));

    m_cloudUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CloudUniform));

    m_smokeUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(SmokeUniform));
  }

  void RayTracer::doRayTracing(const RenderInfo* renderInfo,
                               const std::shared_ptr<PipelineManager>& pipelineManager,
                               const std::shared_ptr<LightingManager>& lightingManager,
                               const ImageResource& imageResource,
                               const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                               const std::shared_ptr<Cloud>& cloud,
                               const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes,
                               const glm::vec3& viewPosition,
                               const glm::mat4& viewMatrix)
  {
    if (renderObjects.empty() && !cloud && smokeVolumes.empty())
    {
      return;
    }

    createTLAS(renderObjects, cloud, smokeVolumes);

    updateRTSceneInfo(renderObjects, smokeVolumes);

    updateRTDescriptorSetData(renderInfo->extent, renderInfo->currentFrame, viewPosition, viewMatrix);

    if (cloud)
    {
      const auto cloudUBO = cloud->getUniformData();
      m_cloudUniform->update(renderInfo->currentFrame, &cloudUBO);
    }

    updateRTDescriptorSets(imageResource, renderInfo->currentFrame);

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
                             const std::shared_ptr<Cloud>& cloud,
                             const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes)
  {
    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }

    m_tlas = nullptr;
    m_tlasBuffer = nullptr;
    m_tlasBufferMemory = nullptr;
    m_tlasInstanceBuffer = nullptr;
    m_tlasInstanceBufferMemory = nullptr;
    m_mergedVertexBuffer = nullptr;
    m_mergedVertexBufferMemory = nullptr;
    m_mergedIndexBuffer = nullptr;
    m_mergedIndexBufferMemory = nullptr;
    m_meshInfoBuffer = nullptr;
    m_meshInfoBufferMemory = nullptr;

    const auto primitiveCount = createTLASInstanceBuffer(renderObjects, cloud, smokeVolumes);

    const vk::AccelerationStructureGeometryInstancesDataKHR instancesData {
      .arrayOfPointers = vk::False,
      .data = m_logicalDevice->getBufferDeviceAddress(m_tlasInstanceBuffer)
    };

    vk::AccelerationStructureGeometryKHR geometry {
      .geometryType = vk::GeometryTypeKHR::eInstances,
      .geometry = instancesData
    };

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {
      .type = vk::AccelerationStructureTypeKHR::eTopLevel,
      .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
      .geometryCount = 1,
      .pGeometries = &geometry
    };

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo{};

    m_logicalDevice->getAccelerationStructureBuildSizes(buildGeometryInfo, primitiveCount, buildSizesInfo);

    Buffers::createBuffer(
      m_logicalDevice,
      buildSizesInfo.accelerationStructureSize,
      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_tlasBuffer,
      m_tlasBufferMemory
    );

    buildTLAS(buildGeometryInfo, buildSizesInfo, primitiveCount);
  }

  uint32_t RayTracer::createTLASInstanceBuffer(const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                               const std::shared_ptr<Cloud>& cloud,
                                               const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes)
  {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    instances.reserve(renderObjects.size());

    populateInstanceArray(instances, renderObjects, cloud, smokeVolumes);

    const vk::DeviceSize instancesBufferSize = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;

    Buffers::createBuffer(
      m_logicalDevice,
      instancesBufferSize,
      vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      stagingBuffer,
      stagingBufferMemory
    );

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [instances, instancesBufferSize](void* data) {
      memcpy(data, instances.data(), instancesBufferSize);
    });

    Buffers::createBuffer(
      m_logicalDevice,
      instancesBufferSize,
      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress |
      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      m_tlasInstanceBuffer,
      m_tlasInstanceBufferMemory
    );

    Buffers::copyBuffer(
      m_logicalDevice,
      m_commandPool,
      m_logicalDevice->getGraphicsQueue(),
      *stagingBuffer,
      *m_tlasInstanceBuffer,
      instancesBufferSize
    );

    return static_cast<uint32_t>(instances.size());
  }

  void RayTracer::populateInstanceArray(std::vector<vk::AccelerationStructureInstanceKHR>& instances,
                                        const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                        const std::shared_ptr<Cloud>& cloud,
                                        const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes) const
  {
    for (const auto& renderObject : renderObjects)
    {
      const glm::mat4 modelMatrix = glm::transpose(renderObject->getModelMatrix());

      vk::TransformMatrixKHR transformMatrix;
      memcpy(&transformMatrix, &modelMatrix, sizeof(vk::TransformMatrixKHR));

      const vk::AccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo {
        .accelerationStructure = renderObject->getModel()->getBLAS()
      };

      const vk::AccelerationStructureInstanceKHR instance {
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
      const vk::AccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo {
        .accelerationStructure = cloud->getBLAS()
      };

      const glm::mat4 modelTransform = glm::scale(glm::translate(glm::mat4(1.0f), cloud->getTranslation()), cloud->getScale());

      const glm::mat4 modelMatrix = glm::transpose(modelTransform);
      vk::TransformMatrixKHR transformMatrix;
      memcpy(&transformMatrix, &modelMatrix, sizeof(vk::TransformMatrixKHR));

      const vk::AccelerationStructureInstanceKHR instance {
        .transform = transformMatrix,
        .instanceCustomIndex = static_cast<uint32_t>(instances.size()),
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 1,
        .flags = 0,
        .accelerationStructureReference = m_logicalDevice->getAccelerationStructureDeviceAddress(&accelerationStructureDeviceAddressInfo)
      };

      instances.push_back(instance);
    }

    for (const auto& volume : smokeVolumes)
    {
      const vk::AccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo {
        .accelerationStructure = volume->getBLAS()
      };

      const glm::mat4 modelTransform = glm::scale(glm::translate(glm::mat4(1.0f), volume->getTranslation()), volume->getScale());

      const glm::mat4 modelMatrix = glm::transpose(modelTransform);
      vk::TransformMatrixKHR transformMatrix;
      memcpy(&transformMatrix, &modelMatrix, sizeof(vk::TransformMatrixKHR));

      const vk::AccelerationStructureInstanceKHR instance {
        .transform = transformMatrix,
        .instanceCustomIndex = static_cast<uint32_t>(instances.size()),
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 2,
        .flags = 0,
        .accelerationStructureReference = m_logicalDevice->getAccelerationStructureDeviceAddress(&accelerationStructureDeviceAddressInfo)
      };

      instances.push_back(instance);
    }
  }

  void RayTracer::buildTLAS(vk::AccelerationStructureBuildGeometryInfoKHR& buildGeometryInfo,
                            const vk::AccelerationStructureBuildSizesInfoKHR& buildSizesInfo,
                            const uint32_t primitiveCount)
  {
    const vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo {
      .buffer = *m_tlasBuffer,
      .size = buildSizesInfo.accelerationStructureSize,
      .type = vk::AccelerationStructureTypeKHR::eTopLevel
    };

    m_tlas = m_logicalDevice->createAccelerationStructure(accelerationStructureCreateInfo);

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

    buildGeometryInfo.dstAccelerationStructure = *m_tlas;
    buildGeometryInfo.scratchData.deviceAddress = m_logicalDevice->getBufferDeviceAddress(*scratchBuffer);

    const vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo {
      .primitiveCount = primitiveCount,
      .primitiveOffset = 0,
      .firstVertex = 0,
      .transformOffset = 0
    };

    const auto commandBuffer = SingleUseCommandBuffer(m_logicalDevice, m_commandPool, m_logicalDevice->getGraphicsQueue());

    commandBuffer.record([&commandBuffer, buildGeometryInfo, buildRangeInfo] {
      commandBuffer.buildAccelerationStructure(buildGeometryInfo, &buildRangeInfo);
    });

    m_tlasInfo = {
      .accelerationStructureCount = 1,
      .pAccelerationStructures = &*m_tlas
    };
  }

  void RayTracer::updateRTSceneInfo(const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                    const std::vector<std::shared_ptr<SmokeVolume>>& smokeVolumes)
  {
    std::vector<Vertex> mergedVertices;
    std::vector<uint32_t> mergedIndices;
    std::vector<MeshInfo> meshInfos;
    std::vector<SmokeUniform> smokeInfos;
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

    if (renderObjects.empty())
    {
      mergedVertices.push_back(Vertex{});

      mergedIndices.push_back(0);

      meshInfos.push_back(MeshInfo{});
    }

    for (const auto& volume : smokeVolumes)
    {
      smokeInfos.push_back(volume->getUniformData());
    }

    uploadRTSceneInfoBuffers(mergedVertices, mergedIndices, meshInfos, smokeInfos);
  }

  void RayTracer::uploadRTSceneInfoBuffers(const std::vector<Vertex>& mergedVertices,
                                           const std::vector<uint32_t>& mergedIndices,
                                           const std::vector<MeshInfo>& meshInfos,
                                           const std::vector<SmokeUniform>& smokeInfos)
  {
    auto uploadBuffer = [&]<typename T>(const std::vector<T>& data,
                                        vk::raii::Buffer& outBuffer,
                                        vk::raii::DeviceMemory& outMemory)
    {
      if (data.empty())
      {
        return;
      }

      const vk::DeviceSize size = data.size() * sizeof(T);

      vk::raii::Buffer stagingBuffer = nullptr;
      vk::raii::DeviceMemory stagingMemory = nullptr;

      Buffers::createBuffer(
        m_logicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingMemory
      );

      Buffers::doMappedMemoryOperation(stagingMemory, [&data, size](void* ptr) {
        memcpy(ptr, data.data(), size);
      });

      Buffers::createBuffer(
        m_logicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        outBuffer,
        outMemory
      );

      Buffers::copyBuffer(
        m_logicalDevice,
        m_commandPool,
        m_logicalDevice->getGraphicsQueue(),
        *stagingBuffer,
        *outBuffer,
        size
      );
    };

    uploadBuffer(mergedVertices, m_mergedVertexBuffer, m_mergedVertexBufferMemory);
    uploadBuffer(mergedIndices, m_mergedIndexBuffer, m_mergedIndexBufferMemory);
    uploadBuffer(meshInfos, m_meshInfoBuffer, m_meshInfoBufferMemory);
    uploadBuffer(smokeInfos, m_smokeInfoBuffer, m_smokeInfoBufferMemory);
  }

  void RayTracer::updateRTDescriptorSets(const ImageResource& imageResource,
                                         const uint32_t currentFrame)
  {
    m_rayTracingDescriptorSet->updateDescriptorSets([this, &imageResource, currentFrame](const vk::DescriptorSet descriptorSet, [[maybe_unused]] const size_t frame)
    {
      auto storageBuffer = [&](const uint32_t binding, const vk::DescriptorBufferInfo* info) {
        return vk::WriteDescriptorSet {
          .dstSet = descriptorSet,
          .dstBinding = binding,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eStorageBuffer,
          .pBufferInfo = info
        };
      };

      std::vector descriptorWrites{{
        {
          .pNext = &m_tlasInfo,
          .dstSet = descriptorSet,
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eAccelerationStructureKHR
        },
        {
          .dstSet = descriptorSet,
          .dstBinding = 1,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eStorageImage,
          .pImageInfo = &imageResource.getDescriptorImageInfo()
        },
        m_cameraUniformRT->getDescriptorSet(2, descriptorSet, currentFrame),
        storageBuffer(3, &m_vertexBufferInfo),
        storageBuffer(4, &m_indexBufferInfo),
        storageBuffer(5, &m_meshInfoInfo),
        storageBuffer(6, &m_smokeInfoInfo),
        m_cloudUniform->getDescriptorSet(7, descriptorSet, currentFrame),
        m_smokeUniform->getDescriptorSet(8, descriptorSet, currentFrame)
      }};

      if (!m_textureImageInfos.empty())
      {
        descriptorWrites.push_back({
          .dstSet = descriptorSet,
          .dstBinding = 9,
          .descriptorCount = static_cast<uint32_t>(m_textureImageInfos.size()),
          .descriptorType = vk::DescriptorType::eCombinedImageSampler,
          .pImageInfo = m_textureImageInfos.data()
        });
      }

      return descriptorWrites;
    });
  }

  void RayTracer::updateRTDescriptorSetData(const vk::Extent2D extent,
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

    m_vertexBufferInfo.buffer = *m_mergedVertexBuffer;
    m_indexBufferInfo.buffer = *m_mergedIndexBuffer;
    m_meshInfoInfo.buffer = *m_meshInfoBuffer;
    m_smokeInfoInfo.buffer = *m_smokeInfoBuffer;
  }

} // vke