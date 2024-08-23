#include "TorusMeshPipeline.h"

TorusMeshPipeline::TorusMeshPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                     const std::shared_ptr<LogicalDevice>& logicalDevice)
  : ComputePipeline(physicalDevice, logicalDevice)
{
}

void TorusMeshPipeline::loadComputeShaders()
{
}
