#include "Pipeline.h"

Pipeline::Pipeline(std::shared_ptr<PhysicalDevice> physicalDevice,
                   std::shared_ptr<LogicalDevice> logicalDevice)
  : physicalDevice(std::move(physicalDevice)), logicalDevice(std::move(logicalDevice))
{}

Pipeline::~Pipeline()
{
  vkDestroyPipeline(logicalDevice->getDevice(), pipeline, nullptr);
  vkDestroyPipelineLayout(logicalDevice->getDevice(), pipelineLayout, nullptr);
}