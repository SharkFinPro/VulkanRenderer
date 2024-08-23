#include "TorusImagePipeline.h"

TorusImagePipeline::TorusImagePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                       const std::shared_ptr<LogicalDevice>& logicalDevice)
  : ComputePipeline(physicalDevice, logicalDevice)
{
}

void TorusImagePipeline::loadComputeShaders()
{
  createShader("assets/torusImage.comp");
}
