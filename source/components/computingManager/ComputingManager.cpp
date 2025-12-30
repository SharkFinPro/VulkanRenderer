#include "ComputingManager.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../pipelines/implementations/DotsPipeline.h"
#include "../pipelines/implementations/SmokePipeline.h"
#include "../pipelines/pipelineManager/PipelineManager.h"
#include "../renderingManager/renderer3D/Renderer3D.h"

namespace vke {

  ComputingManager::ComputingManager(std::shared_ptr<LogicalDevice> logicalDevice, VkCommandPool commandPool)
    : m_logicalDevice(std::move(logicalDevice))
  {
    m_computeCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, commandPool);
  }

  void ComputingManager::doComputing(const std::shared_ptr<PipelineManager>& pipelineManager,
                                     const uint32_t currentFrame,
                                     const std::shared_ptr<Renderer3D>& renderer3D) const
  {
    m_logicalDevice->waitForComputeFences(currentFrame);

    m_logicalDevice->resetComputeFences(currentFrame);

    m_computeCommandBuffer->setCurrentFrame(currentFrame);
    m_computeCommandBuffer->resetCommandBuffer();
    recordComputeCommandBuffer(pipelineManager, currentFrame, renderer3D);

    m_logicalDevice->submitComputeQueue(currentFrame, m_computeCommandBuffer->getCommandBuffer());
  }

  void ComputingManager::recordComputeCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                                    uint32_t currentFrame,
                                                    const std::shared_ptr<Renderer3D>& renderer3D) const
  {
    m_computeCommandBuffer->record([this, pipelineManager, currentFrame, renderer3D]()
    {
      pipelineManager->computeDotsPipeline(m_computeCommandBuffer, currentFrame);

      pipelineManager->computeSmokePipeline(m_computeCommandBuffer, currentFrame, &renderer3D->getSmokeSystems());
    });
  }

} // namespace vke
