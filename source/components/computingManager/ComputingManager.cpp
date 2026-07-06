#include "ComputingManager.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../pipelines/pipelineManager/PipelineManager.h"
#include "../renderingManager/FrameScheduler.h"
#include "../renderingManager/renderer2D/Renderer2D.h"
#include "../renderingManager/renderer3D/Renderer3D.h"

namespace vke {

  ComputingManager::ComputingManager(std::shared_ptr<LogicalDevice> logicalDevice,
                                     std::shared_ptr<FrameScheduler> frameScheduler)
    : m_logicalDevice(std::move(logicalDevice)), m_frameScheduler(std::move(frameScheduler))
  {
    createCommandPool();

    m_computeCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);
  }

  void ComputingManager::doComputing(const std::shared_ptr<PipelineManager>& pipelineManager,
                                     const uint32_t currentFrame,
                                     const std::shared_ptr<Renderer2D>& renderer2D,
                                     const std::shared_ptr<Renderer3D>& renderer3D) const
  {
    // FrameScheduler::beginFrame() has already waited for the previous use of this
    // frame-in-flight slot, so the command buffer is safe to reset.
    m_computeCommandBuffer->setCurrentFrame(currentFrame);
    m_computeCommandBuffer->resetCommandBuffer();
    recordComputeCommandBuffer(pipelineManager, currentFrame, renderer2D, renderer3D);

    m_frameScheduler->submitComputeCommandBuffer(m_computeCommandBuffer->getCommandBuffer());
  }

  void ComputingManager::recordComputeCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                    uint32_t currentFrame,
                                                    const std::shared_ptr<Renderer2D>& renderer2D,
                                                    const std::shared_ptr<Renderer3D>& renderer3D) const
  {
    m_computeCommandBuffer->record([this, pipelineManager, currentFrame, renderer2D, renderer3D] {
      if (renderer2D->shouldDoDots())
      {
        pipelineManager->computeDotsPipeline(m_computeCommandBuffer, currentFrame);
      }

      pipelineManager->computeSmokePipeline(m_computeCommandBuffer, currentFrame, &renderer3D->getSmokeSystems());
    });
  }

  void ComputingManager::createCommandPool()
  {
    const vk::CommandPoolCreateInfo poolInfo {
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer ,
      .queueFamilyIndex = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().computeFamily.value()
    };

    m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
  }
} // namespace vke
