#include "TorusPipeline.h"

TorusPipeline::TorusPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                             const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent)
{
  initMesh();

  initPipelines();
}

void TorusPipeline::initPipelines()
{
  torusMeshPipeline = std::make_unique<TorusMeshPipeline>();
  torusImagePipeline = std::make_unique<TorusImagePipeline>();
  torusDisplayPipeline = std::make_unique<TorusDisplayPipeline>();
  torusDisplayColorPipeline = std::make_unique<TorusDisplayColorPipeline>();
}

void TorusPipeline::initMesh()
{
  std::vector<uint32_t> indicesGen((numU - 1) * (numV - 1) * 6);
  generateIndices(indicesGen.data());

  constexpr uint32_t vertexBuffersSize = 12 * sizeof(float) * numU * numV;
  constexpr uint32_t indexBuffersSize = 6 * sizeof(float) * (numU - 1) * (numV - 1);
  constexpr uint32_t bufferSize = vertexBuffersSize + indexBuffersSize;


}
