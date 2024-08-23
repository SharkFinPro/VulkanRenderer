#ifndef TORUSDISPLAYPIPELINE_H
#define TORUSDISPLAYPIPELINE_H

#include <vulkan/vulkan.h>

#include "../../components/PhysicalDevice.h"
#include "../../components/LogicalDevice.h"

#include "../GraphicsPipeline.h"
#include "../RenderPass.h"


class TorusDisplayPipeline final : public GraphicsPipeline {
public:
  TorusDisplayPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                       const std::shared_ptr<LogicalDevice>& logicalDevice,
                       const std::shared_ptr<RenderPass>& renderPass);

private:
  void loadGraphicsShaders() override;
};



#endif //TORUSDISPLAYPIPELINE_H
