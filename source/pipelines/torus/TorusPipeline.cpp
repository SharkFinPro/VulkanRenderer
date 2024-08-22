#include "TorusPipeline.h"

TorusPipeline::TorusPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                             const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent)
    : ComputePipeline(physicalDevice, logicalDevice), GraphicsPipeline(physicalDevice, logicalDevice)
{
}

void TorusPipeline::loadComputeShaders()
{
}

void TorusPipeline::loadGraphicsShaders()
{
}

void TorusPipeline::initMesh()
{
  std::vector<uint32_t> indicesGen((numU - 1) * (numV - 1) * 6);
  generateIndices(indicesGen.data());

  constexpr uint32_t vertexBuffersSize = 12 * sizeof(float) * numU * numV;
  constexpr uint32_t indexBuffersSize = 6 * sizeof(float) * (numU - 1) * (numV - 1);
  constexpr uint32_t bufferSize = vertexBuffersSize + indexBuffersSize;


}
