#include "RayTracer.h"
#include "../ImageResource.h"
#include "../../assets/AssetManager.h"
#include "../../assets/objects/Cloud.h"
#include "../../assets/objects/Model.h"
#include "../../assets/objects/RenderObject.h"
#include "../../assets/textures/Texture.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../lighting/LightingManager.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../physicalDevice/PhysicalDevice.h"
#include "../../pipelines/GraphicsPipeline.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/implementations/vertexInputs/Vertex.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include "../../../utilities/Buffers.h"
#include <cstring>
#include <unordered_map>

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
    createFrameResources();

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
  }

  void RayTracer::doRayTracing(const RenderInfo* renderInfo,
                               const std::shared_ptr<PipelineManager>& pipelineManager,
                               const std::shared_ptr<LightingManager>& lightingManager,
                               const ImageResource& imageResource,
                               const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                               const std::shared_ptr<Cloud>& cloud,
                               const glm::vec3& viewPosition,
                               const glm::mat4& viewMatrix)
  {
    if (renderObjects.empty() && !cloud)
    {
      return;
    }

    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }

    const uint32_t currentFrame = renderInfo->currentFrame;
    auto& frame = m_frames.at(currentFrame);

    // Release what this slot retired maxFramesInFlight frames ago. beginFrame() has already waited
    // for that frame, so nothing can still be referencing it.
    m_retired.at(currentFrame).buffers.clear();
    m_retired.at(currentFrame).memories.clear();

    updateSceneGeometry(renderInfo->commandBuffer, currentFrame, renderObjects);

    refreshMeshInfoMaterials(currentFrame, frame, renderObjects);

    buildTLAS(renderInfo->commandBuffer, currentFrame, frame, renderObjects, cloud);

    updateRTDescriptorSetData(renderInfo->extent, currentFrame, viewPosition, viewMatrix);

    if (cloud)
    {
      const auto cloudUBO = cloud->getUniformData();
      m_cloudUniform->update(currentFrame, &cloudUBO);
    }

    updateRTDescriptorSets(imageResource, currentFrame, frame);

    pipelineManager->bindRayTracingPipelineDescriptorSet(
      renderInfo->commandBuffer,
      m_rayTracingDescriptorSet->getDescriptorSet(currentFrame),
      0
    );

    pipelineManager->bindRayTracingPipelineDescriptorSet(
      renderInfo->commandBuffer,
      lightingManager->getLightingDescriptorSet()->getDescriptorSet(currentFrame),
      1
    );

    pipelineManager->doRayTracing(renderInfo->commandBuffer, renderInfo->extent);
  }

  void RayTracer::createFrameResources()
  {
    const auto maxFramesInFlight = m_logicalDevice->getMaxFramesInFlight();

    m_frames.resize(maxFramesInFlight);
    m_retired.resize(maxFramesInFlight);
  }

  bool RayTracer::updateSceneGeometry(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                      const uint32_t currentFrame,
                                      const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
  {
    // The merged buffers and the mesh-info layout are derived entirely from the ordered models and
    // their textures, all of which are immutable once loaded.
    std::vector<const void*> signature;
    signature.reserve(renderObjects.size() * 3);

    for (const auto& renderObject : renderObjects)
    {
      signature.push_back(renderObject->getModel().get());
      signature.push_back(renderObject->getTexture().get());
      signature.push_back(renderObject->getSpecularMap().get());
    }

    if (signature == m_sceneSignature)
    {
      return false;
    }

    m_sceneSignature = std::move(signature);

    std::vector<Vertex> mergedVertices;
    std::vector<uint32_t> mergedIndices;

    m_meshInfos.clear();
    m_textureImageInfos.clear();

    std::unordered_map<const Texture*, uint32_t> textureIndices;

    auto textureIndexFor = [&](const std::shared_ptr<Texture>& texture) {
      const auto it = textureIndices.find(texture.get());
      if (it != textureIndices.end())
      {
        return it->second;
      }

      const auto index = static_cast<uint32_t>(textureIndices.size());
      textureIndices.emplace(texture.get(), index);
      m_textureImageInfos.push_back(texture->getImageInfo());

      return index;
    };

    for (const auto& renderObject : renderObjects)
    {
      const auto& model = renderObject->getModel();

      const uint32_t textureIndex = textureIndexFor(renderObject->getTexture());
      const uint32_t specularIndex = textureIndexFor(renderObject->getSpecularMap());

      m_meshInfos.push_back({
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

      m_meshInfos.push_back(MeshInfo{});
    }

    // Staged through the frame's own command buffer, so the upload costs no CPU wait. The staging
    // buffers are retired into this slot and outlive the copy.
    auto upload = [&]<typename T>(const std::vector<T>& data,
                                  vk::raii::Buffer& outBuffer,
                                  vk::raii::DeviceMemory& outMemory,
                                  vk::DeviceSize& outSize,
                                  vk::DescriptorBufferInfo& outInfo)
    {
      const vk::DeviceSize dataSize = data.size() * sizeof(T);

      vk::raii::Buffer stagingBuffer = nullptr;
      vk::raii::DeviceMemory stagingMemory = nullptr;

      Buffers::createBuffer(
        m_logicalDevice,
        dataSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingMemory
      );

      Buffers::doMappedMemoryOperation(stagingMemory, [&data, dataSize](void* ptr) {
        memcpy(ptr, data.data(), dataSize);
      });

      ensureBuffer(
        currentFrame,
        dataSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        outBuffer,
        outMemory,
        outSize
      );

      outInfo = { *outBuffer, 0, vk::WholeSize };

      const vk::BufferCopy copyRegion { .size = dataSize };
      commandBuffer->copyBuffer(*stagingBuffer, *outBuffer, { copyRegion });

      retire(currentFrame, std::move(stagingBuffer), std::move(stagingMemory));
    };

    upload(mergedVertices, m_mergedVertexBuffer, m_mergedVertexBufferMemory, m_mergedVertexBufferSize, m_vertexBufferInfo);
    upload(mergedIndices, m_mergedIndexBuffer, m_mergedIndexBufferMemory, m_mergedIndexBufferSize, m_indexBufferInfo);

    const vk::MemoryBarrier2 uploadBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
      .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
      .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead
    };

    const vk::DependencyInfo dependencyInfo {
      .memoryBarrierCount = 1,
      .pMemoryBarriers = &uploadBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);

    // Every slot's descriptor set points at the buffers that were just replaced.
    for (auto& frame : m_frames)
    {
      frame.descriptorsDirty = true;
    }

    return true;
  }

  void RayTracer::refreshMeshInfoMaterials(const uint32_t currentFrame,
                                           FrameResources& frame,
                                           const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
  {
    // Only the material values change frame to frame; the offsets and texture indices are part of
    // the cached layout built by updateSceneGeometry.
    for (size_t i = 0; i < renderObjects.size() && i < m_meshInfos.size(); ++i)
    {
      m_meshInfos[i].reflectivity = renderObjects[i]->getReflectivity();
      m_meshInfos[i].refractivity = renderObjects[i]->getRefractivity();
      m_meshInfos[i].indexOfRefraction = renderObjects[i]->getIndexOfRefraction();
    }

    const vk::DeviceSize size = m_meshInfos.size() * sizeof(MeshInfo);

    if (ensureBuffer(
          currentFrame,
          size,
          vk::BufferUsageFlagBits::eStorageBuffer,
          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
          frame.meshInfoBuffer,
          frame.meshInfoBufferMemory,
          frame.meshInfoBufferSize,
          &frame.meshInfoMapped))
    {
      frame.meshInfoBufferInfo = { *frame.meshInfoBuffer, 0, vk::WholeSize };
      frame.descriptorsDirty = true;
    }

    memcpy(frame.meshInfoMapped, m_meshInfos.data(), size);
  }

  void RayTracer::buildTLAS(const std::shared_ptr<CommandBuffer>& commandBuffer,
                            const uint32_t currentFrame,
                            FrameResources& frame,
                            const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                            const std::shared_ptr<Cloud>& cloud)
  {
    std::vector<vk::AccelerationStructureInstanceKHR> instances;
    instances.reserve(renderObjects.size() + 1);

    populateInstanceArray(instances, renderObjects, cloud);

    const auto primitiveCount = static_cast<uint32_t>(instances.size());
    const vk::DeviceSize instancesSize = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);

    ensureBuffer(
      currentFrame,
      instancesSize,
      vk::BufferUsageFlagBits::eShaderDeviceAddress |
      vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      frame.instanceBuffer,
      frame.instanceBufferMemory,
      frame.instanceBufferSize,
      &frame.instanceMapped
    );

    // Host writes issued before the queue submit are visible to the build; no barrier needed.
    memcpy(frame.instanceMapped, instances.data(), instancesSize);

    const vk::AccelerationStructureGeometryInstancesDataKHR instancesData {
      .arrayOfPointers = vk::False,
      .data = m_logicalDevice->getBufferDeviceAddress(*frame.instanceBuffer)
    };

    vk::AccelerationStructureGeometryKHR geometry {
      .geometryType = vk::GeometryTypeKHR::eInstances,
      .geometry = instancesData
    };

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {
      .type = vk::AccelerationStructureTypeKHR::eTopLevel,
      .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
      .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
      .geometryCount = 1,
      .pGeometries = &geometry
    };

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo {};

    m_logicalDevice->getAccelerationStructureBuildSizes(buildGeometryInfo, primitiveCount, buildSizesInfo);

    // The acceleration structure object is only recreated when its storage has to grow; otherwise
    // the same TLAS is rebuilt in place, which is what keeps the steady state allocation-free.
    if (ensureBuffer(
          currentFrame,
          buildSizesInfo.accelerationStructureSize,
          vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
          vk::MemoryPropertyFlagBits::eDeviceLocal,
          frame.tlasBuffer,
          frame.tlasBufferMemory,
          frame.tlasBufferSize))
    {
      const vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo {
        .buffer = *frame.tlasBuffer,
        .size = buildSizesInfo.accelerationStructureSize,
        .type = vk::AccelerationStructureTypeKHR::eTopLevel
      };

      frame.tlas = m_logicalDevice->createAccelerationStructure(accelerationStructureCreateInfo);
      frame.descriptorsDirty = true;
    }

    ensureBuffer(
      currentFrame,
      buildSizesInfo.buildScratchSize,
      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      frame.scratchBuffer,
      frame.scratchBufferMemory,
      frame.scratchBufferSize
    );

    buildGeometryInfo.dstAccelerationStructure = *frame.tlas;
    buildGeometryInfo.scratchData.deviceAddress = m_logicalDevice->getBufferDeviceAddress(*frame.scratchBuffer);

    const vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo {
      .primitiveCount = primitiveCount,
      .primitiveOffset = 0,
      .firstVertex = 0,
      .transformOffset = 0
    };

    commandBuffer->buildAccelerationStructure(buildGeometryInfo, &buildRangeInfo);

    const vk::MemoryBarrier2 buildBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
      .srcAccessMask = vk::AccessFlagBits2::eAccelerationStructureWriteKHR,
      .dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
      .dstAccessMask = vk::AccessFlagBits2::eAccelerationStructureReadKHR
    };

    const vk::DependencyInfo dependencyInfo {
      .memoryBarrierCount = 1,
      .pMemoryBarriers = &buildBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void RayTracer::populateInstanceArray(std::vector<vk::AccelerationStructureInstanceKHR>& instances,
                                        const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                        const std::shared_ptr<Cloud>& cloud) const
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
  }

  void RayTracer::updateRTDescriptorSets(const ImageResource& imageResource,
                                         const uint32_t currentFrame,
                                         FrameResources& frame)
  {
    const vk::Image storageImage = imageResource.getImage();

    if (frame.boundStorageImage != storageImage)
    {
      frame.boundStorageImage = storageImage;
      frame.descriptorsDirty = true;
    }

    if (!frame.descriptorsDirty)
    {
      return;
    }

    frame.descriptorsDirty = false;

    frame.tlasInfo = {
      .accelerationStructureCount = 1,
      .pAccelerationStructures = &*frame.tlas
    };

    // Only this slot's set is written. The other slots may still be executing, and their sets
    // point at their own TLAS and mesh-info buffers anyway.
    m_rayTracingDescriptorSet->updateDescriptorSet(currentFrame,
      [this, &imageResource, currentFrame, &frame](const vk::DescriptorSet descriptorSet)
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
          .pNext = &frame.tlasInfo,
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
        storageBuffer(5, &frame.meshInfoBufferInfo),
        m_cloudUniform->getDescriptorSet(6, descriptorSet, currentFrame)
      }};

      if (!m_textureImageInfos.empty())
      {
        descriptorWrites.push_back({
          .dstSet = descriptorSet,
          .dstBinding = 7,
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
                                            const glm::mat4& viewMatrix) const
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
  }

  bool RayTracer::ensureBuffer(const uint32_t currentFrame,
                               const vk::DeviceSize size,
                               const vk::BufferUsageFlags usage,
                               const vk::MemoryPropertyFlags properties,
                               vk::raii::Buffer& buffer,
                               vk::raii::DeviceMemory& memory,
                               vk::DeviceSize& currentSize,
                               void** mapped)
  {
    const vk::DeviceSize requiredSize = std::max<vk::DeviceSize>(size, 1);

    if (currentSize >= requiredSize)
    {
      return false;
    }

    retire(currentFrame, std::move(buffer), std::move(memory));

    Buffers::createBuffer(m_logicalDevice, requiredSize, usage, properties, buffer, memory);

    currentSize = requiredSize;

    if (mapped)
    {
      *mapped = memory.mapMemory(0, vk::WholeSize);
    }

    return true;
  }

  void RayTracer::retire(const uint32_t currentFrame,
                         vk::raii::Buffer&& buffer,
                         vk::raii::DeviceMemory&& memory)
  {
    auto& retired = m_retired.at(currentFrame);

    retired.buffers.push_back(std::move(buffer));
    retired.memories.push_back(std::move(memory));
  }

} // vke
