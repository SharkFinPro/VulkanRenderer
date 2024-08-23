#ifndef TORUSDISPLAYCOLORPIPELINE_H
#define TORUSDISPLAYCOLORPIPELINE_H

#include <vulkan/vulkan.h>

#include "../../components/PhysicalDevice.h"
#include "../../components/LogicalDevice.h"

#include "../GraphicsPipeline.h"
#include "../RenderPass.h"


class TorusDisplayColorPipeline : public GraphicsPipeline {
public:
  TorusDisplayColorPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                            const std::shared_ptr<LogicalDevice>& logicalDevice,
                            const std::shared_ptr<RenderPass>& renderPass);

private:
  void loadGraphicsShaders() override;
};



#endif //TORUSDISPLAYCOLORPIPELINE_H
