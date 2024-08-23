#ifndef TORUSIMAGEPIPELINE_H
#define TORUSIMAGEPIPELINE_H

#include <vulkan/vulkan.h>

#include "../../components/PhysicalDevice.h"
#include "../../components/LogicalDevice.h"

#include "../ComputePipeline.h"


class TorusImagePipeline : public ComputePipeline {
public:
  TorusImagePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                     const std::shared_ptr<LogicalDevice>& logicalDevice);

private:
  void loadComputeShaders() override;
};



#endif //TORUSIMAGEPIPELINE_H
