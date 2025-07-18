#include "Pipeline.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "../core/physicalDevice/PhysicalDevice.h"
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