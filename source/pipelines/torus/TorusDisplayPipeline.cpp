#include "TorusDisplayPipeline.h"

TorusDisplayPipeline::TorusDisplayPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                           const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           const std::shared_ptr<RenderPass>& renderPass)
  : GraphicsPipeline(physicalDevice, logicalDevice)
{
}

void TorusDisplayPipeline::loadGraphicsShaders()
{
}
