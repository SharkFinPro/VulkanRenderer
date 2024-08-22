#include "TorusPipeline.h"

TorusPipeline::TorusPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::shared_ptr<LogicalDevice>& logicalDevice, const VkCommandPool& commandPool,
                             const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent)
    : ComputePipeline(physicalDevice, logicalDevice), GraphicsPipeline(physicalDevice, logicalDevice)
{
}
