#include "TorusPipeline.h"

TorusPipeline::TorusPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                             const std::shared_ptr<RenderPass>& renderPass, const VkExtent2D& swapChainExtent)
{
  initMesh();

  initPipelines(physicalDevice, logicalDevice, renderPass);
}

void TorusPipeline::initPipelines(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                  const std::shared_ptr<LogicalDevice>& logicalDevice,
                                  const std::shared_ptr<RenderPass>& renderPass)
{
  torusMeshPipeline = std::make_unique<TorusMeshPipeline>(physicalDevice, logicalDevice);
  torusImagePipeline = std::make_unique<TorusTexturePipeline>(physicalDevice, logicalDevice);
  torusDisplayPipeline = std::make_unique<TorusDisplayPipeline>(physicalDevice, logicalDevice, renderPass);
  torusDisplayColorPipeline = std::make_unique<TorusDisplayColorPipeline>(physicalDevice, logicalDevice, renderPass);
}

void TorusPipeline::initMesh()
{
  std::vector<uint32_t> indicesGen((numU - 1) * (numV - 1) * 6);
  generateIndices(indicesGen.data());

  constexpr uint32_t vertexBuffersSize = 12 * sizeof(float) * numU * numV;
  constexpr uint32_t indexBuffersSize = 6 * sizeof(float) * (numU - 1) * (numV - 1);
  constexpr uint32_t bufferSize = vertexBuffersSize + indexBuffersSize;


}
