#include "ComputingManager.h"
#include "../core/commandBuffer/CommandBuffer.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "../PipelineManager.h"
#include "../../pipelines/custom/DotsPipeline.h"
#include "../../pipelines/custom/SmokePipeline.h"

ComputingManager::ComputingManager(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool)
  : m_logicalDevice(logicalDevice)
{
  m_computeCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, commandPool);
}

void ComputingManager::doComputing(const std::shared_ptr<PipelineManager>& pipelineManager,
                                   const uint32_t currentFrame) const
{
  m_logicalDevice->waitForComputeFences(currentFrame);

  m_logicalDevice->resetComputeFences(currentFrame);

  m_computeCommandBuffer->setCurrentFrame(currentFrame);
  m_computeCommandBuffer->resetCommandBuffer();
  recordComputeCommandBuffer(pipelineManager, currentFrame);

  m_logicalDevice->submitComputeQueue(currentFrame, m_computeCommandBuffer->getCommandBuffer());
}

void ComputingManager::recordComputeCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                  uint32_t currentFrame) const
{
  m_computeCommandBuffer->record([this, pipelineManager, currentFrame]()
  {
    if (const auto dotsPipeline = pipelineManager->getDotsPipeline())
    {
      dotsPipeline->compute(m_computeCommandBuffer, currentFrame);
    }

    for (const auto& system : pipelineManager->getSmokeSystems())
    {
      system->compute(m_computeCommandBuffer, currentFrame);
    }
  });
}
