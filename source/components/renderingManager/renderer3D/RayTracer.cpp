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
#include "../../pipelines/GraphicsPipeline.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/implementations/vertexInputs/Vertex.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include "../../../utilities/Buffers.h"
#include <algorithm>
#include <bit>
#include <cstring>

namespace vke {

  struct CameraUniformRT {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    glm::mat4 projection;
    glm::vec3 viewPosition;
    // std140 packs these straight after the vec3, matching this struct's layout.
    float nearPlane;
    float farPlane;
  };

  namespace {
    // Smallest per-frame buffer, in elements, so a scene that grows one object at a time doesn't
    // reallocate every frame at first.
    constexpr uint32_t s_minimumCapacity = 16;

    uint32_t growCapacity(const size_t count)
    {
      return std::bit_ceil(std::max(static_cast<uint32_t>(count), s_minimumCapacity));
    }
  }

  RayTracer::RayTracer(std::shared_ptr<LogicalDevice> logicalDevice,
                       const std::shared_ptr<AssetManager>& assetManager,
                       const vk::DescriptorPool descriptorPool)
    : m_logicalDevice(std::move(logicalDevice))
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

    m_frameResources.resize(m_logicalDevice->getMaxFramesInFlight());
  }

  bool RayTracer::doRayTracing(const RenderInfo* renderInfo,
                               const std::shared_ptr<PipelineManager>& pipelineManager,
                               const std::shared_ptr<LightingManager>& lightingManager,
                               const ImageResource& imageResource,
                               const ImageResource& depthImageResource,
                               const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                               const std::shared_ptr<Cloud>& cloud)
  {
    if (renderObjects.empty() && !cloud)
    {
      return false;
    }

    auto& frame = m_frameResources[renderInfo->currentFrame];

    const bool copiedGeometry = updateMergedGeometry(renderInfo, renderObjects);

    updateMeshInfos(frame, renderObjects);

    buildTLAS(renderInfo, renderObjects, cloud);

    // Host writes (instances, mesh infos) are made visible to the device by the queue submission itself.
    const std::array memoryBarriers {
      vk::MemoryBarrier2 {
        .srcStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .srcAccessMask = vk::AccessFlagBits2::eAccelerationStructureWriteKHR,
        .dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .dstAccessMask = vk::AccessFlagBits2::eAccelerationStructureReadKHR
      },
      vk::MemoryBarrier2 {
        .srcStageMask = vk::PipelineStageFlagBits2::eCopy,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead
      }
    };

    const vk::DependencyInfo dependencyInfo {
      .memoryBarrierCount = copiedGeometry ? 2u : 1u,
      .pMemoryBarriers = memoryBarriers.data()
    };

    renderInfo->commandBuffer->pipelineBarrier(dependencyInfo);

    updateRTDescriptorSetData(renderInfo);

    if (cloud)
    {
      const auto cloudUBO = cloud->getUniformData();
      m_cloudUniform->update(renderInfo->currentFrame, &cloudUBO);
    }

    updateRTDescriptorSet(imageResource, depthImageResource, renderInfo->currentFrame);

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

    return true;
  }

  void RayTracer::buildTLAS(const RenderInfo* renderInfo,
                            const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                            const std::shared_ptr<Cloud>& cloud)
  {
    auto& frame = m_frameResources[renderInfo->currentFrame];

    const auto instanceCount = static_cast<uint32_t>(renderObjects.size() + (cloud ? 1 : 0));

    reserveTLAS(frame, instanceCount);

    populateInstances(static_cast<vk::AccelerationStructureInstanceKHR*>(frame.instances.data), renderObjects, cloud);

    const vk::AccelerationStructureGeometryInstancesDataKHR instancesData {
      .arrayOfPointers = vk::False,
      .data = frame.instancesAddress
    };

    const vk::AccelerationStructureGeometryKHR geometry {
      .geometryType = vk::GeometryTypeKHR::eInstances,
      .geometry = instancesData
    };

    // Flags must match the ones reserveTLAS queried the sizes with.
    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {
      .type = vk::AccelerationStructureTypeKHR::eTopLevel,
      .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
      .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
      .dstAccelerationStructure = *frame.tlas,
      .geometryCount = 1,
      .pGeometries = &geometry
    };
    buildGeometryInfo.scratchData.deviceAddress = frame.scratchAddress;

    const vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo {
      .primitiveCount = instanceCount,
      .primitiveOffset = 0,
      .firstVertex = 0,
      .transformOffset = 0
    };

    renderInfo->commandBuffer->buildAccelerationStructure(buildGeometryInfo, &buildRangeInfo);
  }

  void RayTracer::reserveTLAS(FrameResources& frame,
                              const uint32_t instanceCount) const
  {
    if (instanceCount <= frame.instanceCapacity)
    {
      return;
    }

    // This frame-in-flight slot's previous frame has completed, so its resources can be destroyed now.
    const uint32_t capacity = growCapacity(instanceCount);

    reserveMappedBuffer(
      frame.instances,
      capacity * sizeof(vk::AccelerationStructureInstanceKHR),
      vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
    );
    frame.instancesAddress = m_logicalDevice->getBufferDeviceAddress(*frame.instances.buffer);

    const vk::AccelerationStructureGeometryKHR geometry {
      .geometryType = vk::GeometryTypeKHR::eInstances,
      .geometry = vk::AccelerationStructureGeometryInstancesDataKHR {
        .arrayOfPointers = vk::False
      }
    };

    const vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo {
      .type = vk::AccelerationStructureTypeKHR::eTopLevel,
      .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
      .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
      .geometryCount = 1,
      .pGeometries = &geometry
    };

    // Sizes for the capacity hold for any build of up to that many instances.
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    m_logicalDevice->getAccelerationStructureBuildSizes(buildGeometryInfo, capacity, buildSizesInfo);

    frame.tlas = nullptr;
    frame.tlasBuffer = {};

    Buffers::createBuffer(
      m_logicalDevice,
      buildSizesInfo.accelerationStructureSize,
      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      frame.tlasBuffer.buffer,
      frame.tlasBuffer.memory
    );

    frame.tlas = m_logicalDevice->createAccelerationStructure({
      .buffer = *frame.tlasBuffer.buffer,
      .size = buildSizesInfo.accelerationStructureSize,
      .type = vk::AccelerationStructureTypeKHR::eTopLevel
    });

    frame.tlasInfo = {
      .accelerationStructureCount = 1,
      .pAccelerationStructures = &*frame.tlas
    };

    if (buildSizesInfo.buildScratchSize > frame.scratchSize)
    {
      frame.scratch = {};

      Buffers::createBuffer(
        m_logicalDevice,
        buildSizesInfo.buildScratchSize,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        frame.scratch.buffer,
        frame.scratch.memory
      );

      frame.scratchSize = buildSizesInfo.buildScratchSize;
      frame.scratchAddress = m_logicalDevice->getBufferDeviceAddress(*frame.scratch.buffer);
    }

    frame.instanceCapacity = capacity;
  }

  void RayTracer::populateInstances(vk::AccelerationStructureInstanceKHR* instances,
                                    const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                    const std::shared_ptr<Cloud>& cloud) const
  {
    uint32_t instanceIndex = 0;

    for (const auto& renderObject : renderObjects)
    {
      const glm::mat4 modelMatrix = glm::transpose(renderObject->getModelMatrix());

      vk::TransformMatrixKHR transformMatrix;
      memcpy(&transformMatrix, &modelMatrix, sizeof(vk::TransformMatrixKHR));

      const vk::AccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo {
        .accelerationStructure = renderObject->getModel()->getBLAS()
      };

      instances[instanceIndex] = {
        .transform = transformMatrix,
        .instanceCustomIndex = instanceIndex,
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = 0,
        .accelerationStructureReference = m_logicalDevice->getAccelerationStructureDeviceAddress(&accelerationStructureDeviceAddressInfo)
      };

      ++instanceIndex;
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

      instances[instanceIndex] = {
        .transform = transformMatrix,
        .instanceCustomIndex = instanceIndex,
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 1,
        .flags = 0,
        .accelerationStructureReference = m_logicalDevice->getAccelerationStructureDeviceAddress(&accelerationStructureDeviceAddressInfo)
      };
    }
  }

  bool RayTracer::updateMergedGeometry(const RenderInfo* renderInfo,
                                       const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
  {
    if (mergedGeometryIsCurrent(renderObjects))
    {
      return false;
    }

    rebuildMergedGeometry(renderInfo, renderObjects);

    return true;
  }

  bool RayTracer::mergedGeometryIsCurrent(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) const
  {
    if (!*m_mergedVertexBuffer.buffer)
    {
      return false;
    }

    return std::ranges::all_of(renderObjects, [this](const auto& renderObject) {
      const auto entry = m_modelGeometry.find(renderObject->getModel().get());

      // An expired entry is a released model; a live model at the same address is a different one.
      return entry != m_modelGeometry.end() && !entry->second.model.expired();
    });
  }

  void RayTracer::rebuildMergedGeometry(const RenderInfo* renderInfo,
                                        const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
  {
    // Only the models traced this frame are kept, so released or unused models drop out here.
    m_modelGeometry.clear();

    std::vector<Vertex> mergedVertices;
    std::vector<uint32_t> mergedIndices;

    for (const auto& renderObject : renderObjects)
    {
      const auto model = renderObject->getModel();

      const auto [entry, inserted] = m_modelGeometry.try_emplace(model.get());
      if (!inserted)
      {
        continue;
      }

      entry->second = {
        .model = model,
        .vertexOffset = static_cast<uint32_t>(mergedVertices.size()),
        .indexOffset = static_cast<uint32_t>(mergedIndices.size())
      };

      const auto& vertices = model->getVertices();
      const auto& indices = model->getIndices();

      mergedVertices.insert(mergedVertices.end(), vertices.begin(), vertices.end());
      mergedIndices.insert(mergedIndices.end(), indices.begin(), indices.end());
    }

    // The descriptors need a buffer even when only a cloud is traced.
    if (mergedVertices.empty())
    {
      mergedVertices.emplace_back();
      mergedIndices.push_back(0);
    }

    // Frames still in flight may be reading the old buffers.
    if (*m_mergedVertexBuffer.buffer)
    {
      m_logicalDevice->retireHandles(std::move(m_mergedVertexBuffer));
      m_logicalDevice->retireHandles(std::move(m_mergedIndexBuffer));
    }

    auto upload = [this, renderInfo]<typename T>(const std::vector<T>& data, DeviceBuffer& outBuffer)
    {
      const vk::DeviceSize size = data.size() * sizeof(T);

      DeviceBuffer stagingBuffer;

      Buffers::createBuffer(
        m_logicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer.buffer,
        stagingBuffer.memory
      );

      Buffers::doMappedMemoryOperation(stagingBuffer.memory, [&data, size](void* ptr) {
        memcpy(ptr, data.data(), size);
      });

      Buffers::createBuffer(
        m_logicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        outBuffer.buffer,
        outBuffer.memory
      );

      renderInfo->commandBuffer->copyBuffer(*stagingBuffer.buffer, *outBuffer.buffer, vk::BufferCopy { .size = size });

      // Kept alive until this frame's copy has run.
      m_logicalDevice->retireHandles(std::move(stagingBuffer));
    };

    upload(mergedVertices, m_mergedVertexBuffer);
    upload(mergedIndices, m_mergedIndexBuffer);

    m_vertexBufferInfo.buffer = *m_mergedVertexBuffer.buffer;
    m_indexBufferInfo.buffer = *m_mergedIndexBuffer.buffer;
  }

  void RayTracer::updateMeshInfos(FrameResources& frame,
                                  const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
  {
    m_textureImageInfos.clear();
    m_textureKeys.clear();

    // The descriptor needs a buffer even when only a cloud is traced.
    const size_t meshCount = std::max<size_t>(renderObjects.size(), 1);

    if (meshCount * sizeof(MeshInfo) > frame.meshInfos.size)
    {
      // This frame-in-flight slot's previous frame has completed, so the old buffer can be destroyed now.
      reserveMappedBuffer(frame.meshInfos, growCapacity(meshCount) * sizeof(MeshInfo), vk::BufferUsageFlagBits::eStorageBuffer);
      frame.meshInfoInfo.buffer = *frame.meshInfos.buffer;
    }

    auto* meshInfos = static_cast<MeshInfo*>(frame.meshInfos.data);

    if (renderObjects.empty())
    {
      meshInfos[0] = MeshInfo{};
      return;
    }

    for (size_t i = 0; i < renderObjects.size(); ++i)
    {
      const auto& renderObject = renderObjects[i];

      // updateMergedGeometry has just made sure every traced model is in the merged buffers.
      const auto& geometry = m_modelGeometry.at(renderObject->getModel().get());

      meshInfos[i] = {
        .vertexOffset = geometry.vertexOffset,
        .indexOffset = geometry.indexOffset,
        .textureIndex = getTextureIndex(renderObject->getTexture()),
        .specularIndex = getTextureIndex(renderObject->getSpecularMap()),
        .reflectivity = renderObject->getReflectivity(),
        .refractivity = renderObject->getRefractivity(),
        .indexOfRefraction = renderObject->getIndexOfRefraction()
      };
    }
  }

  uint32_t RayTracer::getTextureIndex(const std::shared_ptr<Texture>& texture)
  {
    const auto entry = std::ranges::find(m_textureKeys, texture.get());

    if (entry != m_textureKeys.end())
    {
      return static_cast<uint32_t>(entry - m_textureKeys.begin());
    }

    m_textureKeys.push_back(texture.get());
    m_textureImageInfos.push_back(texture->getImageInfo());

    return static_cast<uint32_t>(m_textureKeys.size() - 1);
  }

  void RayTracer::reserveMappedBuffer(MappedBuffer& mappedBuffer,
                                      const vk::DeviceSize size,
                                      const vk::BufferUsageFlags usage) const
  {
    if (size <= mappedBuffer.size)
    {
      return;
    }

    // Freeing the memory unmaps it.
    mappedBuffer = {};

    Buffers::createBuffer(
      m_logicalDevice,
      size,
      usage,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      mappedBuffer.buffer,
      mappedBuffer.memory
    );

    mappedBuffer.data = mappedBuffer.memory.mapMemory(0, vk::WholeSize);
    mappedBuffer.size = size;
  }

  void RayTracer::updateRTDescriptorSet(const ImageResource& imageResource,
                                        const ImageResource& depthImageResource,
                                        const uint32_t currentFrame)
  {
    // Only this frame's set: the others may still be in use by frames in flight.
    m_rayTracingDescriptorSet->updateDescriptorSet(currentFrame, [this, &imageResource, &depthImageResource, currentFrame](const vk::DescriptorSet descriptorSet, [[maybe_unused]] const size_t frame)
    {
      const auto& frameResources = m_frameResources[currentFrame];

      auto storageBuffer = [&](const uint32_t binding, const vk::DescriptorBufferInfo* info) {
        return vk::WriteDescriptorSet {
          .dstSet = descriptorSet,
          .dstBinding = binding,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eStorageBuffer,
          .pBufferInfo = info
        };
      };

      auto& descriptorWrites = m_descriptorWrites;
      descriptorWrites = {{
        {
          .pNext = &frameResources.tlasInfo,
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
        storageBuffer(5, &frameResources.meshInfoInfo),
        m_cloudUniform->getDescriptorSet(6, descriptorSet, currentFrame),
        {
          .dstSet = descriptorSet,
          .dstBinding = 7,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eStorageImage,
          .pImageInfo = &depthImageResource.getDescriptorImageInfo()
        }
      }};

      size_t count = 8;
      if (!m_textureImageInfos.empty())
      {
        descriptorWrites[count++] = {
          .dstSet = descriptorSet,
          .dstBinding = 8,
          .descriptorCount = static_cast<uint32_t>(m_textureImageInfos.size()),
          .descriptorType = vk::DescriptorType::eCombinedImageSampler,
          .pImageInfo = m_textureImageInfos.data()
        };
      }

      return vk::ArrayProxy<const vk::WriteDescriptorSet>(static_cast<uint32_t>(count), descriptorWrites.data());
    });
  }

  void RayTracer::updateRTDescriptorSetData(const RenderInfo* renderInfo)
  {
    const auto projectionMatrix = renderInfo->getProjectionMatrix();

    const CameraUniformRT cameraUBORT {
      .viewInverse = glm::inverse(renderInfo->viewMatrix),
      .projInverse = glm::inverse(projectionMatrix),
      .projection = projectionMatrix,
      .viewPosition = renderInfo->viewPosition,
      .nearPlane = renderInfo->nearPlane,
      .farPlane = renderInfo->farPlane
    };

    m_cameraUniformRT->update(renderInfo->currentFrame, &cameraUBORT);
  }

} // vke
