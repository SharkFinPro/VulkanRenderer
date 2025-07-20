#include "Pipeline.h"
#include "../core/logicalDevice/LogicalDevice.h"

Pipeline::Pipeline(const std::shared_ptr<LogicalDevice>& logicalDevice)
  : m_logicalDevice(logicalDevice)
{}

Pipeline::~Pipeline()
{
  m_logicalDevice->destroyPipeline(m_pipeline);

  m_logicalDevice->destroyPipelineLayout(m_pipelineLayout);
}