#include "TorusDisplayColorPipeline.h"

TorusDisplayColorPipeline::TorusDisplayColorPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                                     const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                     const std::shared_ptr<RenderPass>& renderPass)
  : GraphicsPipeline(physicalDevice, logicalDevice)
{
}

void TorusDisplayColorPipeline::loadGraphicsShaders()
{
}
