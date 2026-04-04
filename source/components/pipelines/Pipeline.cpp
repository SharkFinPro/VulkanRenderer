#include "Pipeline.h"

namespace vke {

  Pipeline::Pipeline(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice))
  {}

} // namespace vke