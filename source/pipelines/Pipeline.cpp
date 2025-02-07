#include "Pipeline.h"
#include "../components/LogicalDevice.h"
#include "../components/PhysicalDevice.h"
#include <iostream>

Pipeline::Pipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                   const std::shared_ptr<LogicalDevice>& logicalDevice)
  : physicalDevice(physicalDevice), logicalDevice(logicalDevice)
{}

Pipeline::~Pipeline()
{
  if (pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(logicalDevice->getDevice(), pipeline, nullptr);
    pipeline = VK_NULL_HANDLE;
  }
  else
  {
    std::cerr << "Warning: Attempted to destroy an invalid pipeline!" << std::endl;
  }

  if (pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(logicalDevice->getDevice(), pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
  }
  else
  {
    std::cerr << "Warning: Attempted to destroy an invalid pipeline layout!" << std::endl;
  }
}