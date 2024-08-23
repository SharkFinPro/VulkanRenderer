#include "TorusDisplayPipeline.h"

TorusDisplayPipeline::TorusDisplayPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                           const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           const std::shared_ptr<RenderPass>& renderPass)
  : GraphicsPipeline(physicalDevice, logicalDevice)
{
}

void TorusDisplayPipeline::loadGraphicsShaders()
{
  createShader("assets/torus.vert", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/torus.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
}
