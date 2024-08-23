#include "TorusTexturePipeline.h"

TorusTexturePipeline::TorusTexturePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                           const std::shared_ptr<LogicalDevice>& logicalDevice)
  : ComputePipeline(physicalDevice, logicalDevice)
{
}

void TorusTexturePipeline::loadComputeShaders()
{
  createShader("assets/torusTexture.comp");
}
