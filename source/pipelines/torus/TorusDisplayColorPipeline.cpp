#include "TorusDisplayColorPipeline.h"

TorusDisplayColorPipeline::TorusDisplayColorPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                                     const std::shared_ptr<LogicalDevice>& logicalDevice,
                                                     const std::shared_ptr<RenderPass>& renderPass)
  : GraphicsPipeline(physicalDevice, logicalDevice)
{
}

void TorusDisplayColorPipeline::loadGraphicsShaders()
{
  createShader("assets/torus.vert", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/torusColor.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
}
