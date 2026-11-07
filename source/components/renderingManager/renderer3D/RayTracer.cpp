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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <utility>

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

    recordTraceInputBarrier(renderInfo, copiedGeometry);

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

  void RayTracer::recordTraceInputBarrier(const RenderInfo* renderInfo,
                                          const bool copiedGeometry)
  {
    // Host writes (instances, mesh infos) are made visible to the device by the queue submission itself. The
    // copy barrier is only recorded in the frame that uploaded the merged geometry; later frames read it
    // without one, which is still ordered because a barrier's second scope covers every later command
    // submitted to the same queue.
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

    const uint32_t capacity = growCapacity(instanceCount);

    // Cleared until everything below exists, so a failed allocation is retried on the next call.
    frame.instanceCapacity = 0;

    reserveMappedBuffer(
      frame.instances,
      capacity * sizeof(vk::AccelerationStructureInstanceKHR),
      vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
    );
    frame.instancesAddress = m_logicalDevice->getBufferDeviceAddress(*frame.instances.buffer);

    // Sizes for the capacity hold for any build of up to that many instances.
    const auto buildSizesInfo = getTLASBuildSizes(capacity);

    recreateTLAS(frame, buildSizesInfo.accelerationStructureSize);

    reserveScratch(frame, buildSizesInfo.buildScratchSize);

    frame.instanceCapacity = capacity;
  }

  vk::AccelerationStructureBuildSizesInfoKHR RayTracer::getTLASBuildSizes(const uint32_t instanceCount) const
  {
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

    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    m_logicalDevice->getAccelerationStructureBuildSizes(buildGeometryInfo, instanceCount, buildSizesInfo);

    return buildSizesInfo;
  }

  void RayTracer::recreateTLAS(FrameResources& frame,
                               const vk::DeviceSize size) const
  {
    // Retired rather than destroyed, so nothing here depends on which frames could still be using them.
    struct RetiredTLAS {
      DeviceBuffer buffer;
      // Declared last so it is destroyed before the buffer backing it.
      vk::raii::AccelerationStructureKHR tlas = nullptr;
    };

    if (*frame.tlas)
    {
      m_logicalDevice->retireHandles(RetiredTLAS { .buffer = std::move(frame.tlasBuffer), .tlas = std::move(frame.tlas) });
    }

    frame.tlas = nullptr;
    frame.tlasBuffer = {};

    Buffers::createBuffer(
      m_logicalDevice,
      size,
      vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      frame.tlasBuffer.buffer,
      frame.tlasBuffer.memory
    );

    frame.tlas = m_logicalDevice->createAccelerationStructure({
      .buffer = *frame.tlasBuffer.buffer,
      .size = size,
      .type = vk::AccelerationStructureTypeKHR::eTopLevel
    });

    frame.tlasInfo = {
      .accelerationStructureCount = 1,
      .pAccelerationStructures = &*frame.tlas
    };
  }

  void RayTracer::reserveScratch(FrameResources& frame,
                                 const vk::DeviceSize size) const
  {
    if (size <= frame.scratchSize)
    {
      return;
    }

    if (*frame.scratch.buffer)
    {
      m_logicalDevice->retireHandles(std::move(frame.scratch));
    }

    frame.scratch = {};
    frame.scratchSize = 0;

    Buffers::createBuffer(
      m_logicalDevice,
      size,
      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      frame.scratch.buffer,
      frame.scratch.memory
    );

    frame.scratchSize = size;
    frame.scratchAddress = m_logicalDevice->getBufferDeviceAddress(*frame.scratch.buffer);
  }

  void RayTracer::populateInstances(vk::AccelerationStructureInstanceKHR* instances,
                                    const std::vector<std::shared_ptr<RenderObject>>& renderObjects,
                                    const std::shared_ptr<Cloud>& cloud) const
  {
    uint32_t instanceIndex = 0;

    for (const auto& renderObject : renderObjects)
    {
      instances[instanceIndex] = makeInstance(renderObject->getModelMatrix(), renderObject->getModel()->getBLAS(), instanceIndex, 0);

      ++instanceIndex;
    }

    if (cloud)
    {
      const glm::mat4 modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), cloud->getTranslation()), cloud->getScale());

      instances[instanceIndex] = makeInstance(modelMatrix, cloud->getBLAS(), instanceIndex, 1);
    }
  }

  vk::AccelerationStructureInstanceKHR RayTracer::makeInstance(const glm::mat4& modelMatrix,
                                                               const vk::AccelerationStructureKHR blas,
                                                               const uint32_t instanceIndex,
                                                               const uint32_t hitGroupOffset) const
  {
    const glm::mat4 rowMajorMatrix = glm::transpose(modelMatrix);

    vk::TransformMatrixKHR transformMatrix;
    std::memcpy(&transformMatrix, &rowMajorMatrix, sizeof(vk::TransformMatrixKHR));

    const vk::AccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo {
      .accelerationStructure = blas
    };

    // Returned by value: assigning a braced list into the mapped element is ambiguous on GCC.
    return {
      .transform = transformMatrix,
      .instanceCustomIndex = instanceIndex,
      .mask = 0xFF,
      .instanceShaderBindingTableRecordOffset = hitGroupOffset,
      .flags = 0,
      .accelerationStructureReference = m_logicalDevice->getAccelerationStructureDeviceAddress(&accelerationStructureDeviceAddressInfo)
    };
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

  std::vector<std::shared_ptr<Model>> RayTracer::collectModelsToMerge(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) const
  {
    // Models merged before stay while they are alive, so a model that stops being traced for a while and
    // comes back doesn't force another rebuild; released models drop out here.
    std::vector<std::shared_ptr<Model>> models;
    models.reserve(m_modelGeometry.size() + renderObjects.size());

    for (const auto& geometry : m_modelGeometry | std::views::values)
    {
      if (auto model = geometry.model.lock())
      {
        models.push_back(std::move(model));
      }
    }

    for (const auto& renderObject : renderObjects)
    {
      models.push_back(renderObject->getModel());
    }

    return models;
  }

  void RayTracer::rebuildMergedGeometry(const RenderInfo* renderInfo,
                                        const std::vector<std::shared_ptr<RenderObject>>& renderObjects)
  {
    const auto models = collectModelsToMerge(renderObjects);

    m_modelGeometry.clear();

    std::vector<Vertex> mergedVertices;
    std::vector<uint32_t> mergedIndices;

    for (const auto& model : models)
    {
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

    // The descriptors need a buffer even when only a cloud is traced, and a buffer can't be empty.
    if (mergedVertices.empty())
    {
      mergedVertices.emplace_back();
    }

    if (mergedIndices.empty())
    {
      mergedIndices.push_back(0);
    }

    // Frames still in flight may be reading the old buffers.
    if (*m_mergedVertexBuffer.buffer)
    {
      m_logicalDevice->retireHandles(std::move(m_mergedVertexBuffer));
      m_logicalDevice->retireHandles(std::move(m_mergedIndexBuffer));
    }

    uploadStorageBuffer(renderInfo, mergedVertices.data(), mergedVertices.size() * sizeof(Vertex), m_mergedVertexBuffer);
    uploadStorageBuffer(renderInfo, mergedIndices.data(), mergedIndices.size() * sizeof(uint32_t), m_mergedIndexBuffer);

    m_vertexBufferInfo.buffer = *m_mergedVertexBuffer.buffer;
    m_indexBufferInfo.buffer = *m_mergedIndexBuffer.buffer;
  }

  void RayTracer::uploadStorageBuffer(const RenderInfo* renderInfo,
                                      const void* data,
                                      const vk::DeviceSize size,
                                      DeviceBuffer& outBuffer) const
  {
    DeviceBuffer stagingBuffer;

    Buffers::createBuffer(
      m_logicalDevice,
      size,
      vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      stagingBuffer.buffer,
      stagingBuffer.memory
    );

    Buffers::doMappedMemoryOperation(stagingBuffer.memory, [data, size](void* ptr) {
      std::memcpy(ptr, data, size);
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

    // Retired rather than destroyed, so nothing here depends on which frames could still be using it.
    // Freeing the memory unmaps it.
    if (*mappedBuffer.buffer)
    {
      m_logicalDevice->retireHandles(std::move(mappedBuffer));
    }

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
      const uint32_t count = fillRTDescriptorWrites(descriptorSet, imageResource, depthImageResource, currentFrame);

      return vk::ArrayProxy<const vk::WriteDescriptorSet>(count, m_descriptorWrites.data());
    });
  }

  uint32_t RayTracer::fillRTDescriptorWrites(const vk::DescriptorSet descriptorSet,
                                             const ImageResource& imageResource,
                                             const ImageResource& depthImageResource,
                                             const uint32_t currentFrame)
  {
    const auto& frameResources = m_frameResources[currentFrame];

    auto write = [descriptorSet](const uint32_t binding, const vk::DescriptorType type) {
      return vk::WriteDescriptorSet {
        .dstSet = descriptorSet,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type
      };
    };

    m_descriptorWrites[0] = write(0, vk::DescriptorType::eAccelerationStructureKHR);
    m_descriptorWrites[0].pNext = &frameResources.tlasInfo;

    m_descriptorWrites[1] = write(1, vk::DescriptorType::eStorageImage);
    m_descriptorWrites[1].pImageInfo = &imageResource.getDescriptorImageInfo();

    m_descriptorWrites[2] = m_cameraUniformRT->getDescriptorSet(2, descriptorSet, currentFrame);

    m_descriptorWrites[3] = write(3, vk::DescriptorType::eStorageBuffer);
    m_descriptorWrites[3].pBufferInfo = &m_vertexBufferInfo;

    m_descriptorWrites[4] = write(4, vk::DescriptorType::eStorageBuffer);
    m_descriptorWrites[4].pBufferInfo = &m_indexBufferInfo;

    m_descriptorWrites[5] = write(5, vk::DescriptorType::eStorageBuffer);
    m_descriptorWrites[5].pBufferInfo = &frameResources.meshInfoInfo;

    m_descriptorWrites[6] = m_cloudUniform->getDescriptorSet(6, descriptorSet, currentFrame);

    m_descriptorWrites[7] = write(7, vk::DescriptorType::eStorageImage);
    m_descriptorWrites[7].pImageInfo = &depthImageResource.getDescriptorImageInfo();

    if (m_textureImageInfos.empty())
    {
      return 8;
    }

    m_descriptorWrites[8] = write(8, vk::DescriptorType::eCombinedImageSampler);
    m_descriptorWrites[8].descriptorCount = static_cast<uint32_t>(m_textureImageInfos.size());
    m_descriptorWrites[8].pImageInfo = m_textureImageInfos.data();

    return 9;
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
