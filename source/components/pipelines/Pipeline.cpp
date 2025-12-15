#include "Pipeline.h"
#include "../logicalDevice/LogicalDevice.h"

namespace vke {

Pipeline::Pipeline(const std::shared_ptr<LogicalDevice>& logicalDevice)
  : m_logicalDevice(logicalDevice)
{}

Pipeline::~Pipeline()
{
  m_logicalDevice->destroyPipeline(m_pipeline);

  m_logicalDevice->destroyPipelineLayout(m_pipelineLayout);
}

} // namespace vke