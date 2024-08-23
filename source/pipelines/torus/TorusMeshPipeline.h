#ifndef TORUSMESHPIPELINE_H
#define TORUSMESHPIPELINE_H

#include <vulkan/vulkan.h>

#include "../../components/PhysicalDevice.h"
#include "../../components/LogicalDevice.h"

#include "../ComputePipeline.h"


class TorusMeshPipeline : public ComputePipeline {
public:
  TorusMeshPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                    const std::shared_ptr<LogicalDevice>& logicalDevice);

private:
  void loadComputeShaders() override;
};



#endif //TORUSMESHPIPELINE_H
