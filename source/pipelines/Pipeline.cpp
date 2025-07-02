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
  logicalDevice->destroyPipeline(pipeline);

  logicalDevice->destroyPipelineLayout(pipelineLayout);
}