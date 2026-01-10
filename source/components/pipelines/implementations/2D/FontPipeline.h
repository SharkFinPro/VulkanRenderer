#ifndef VULKANPROJECT_FONTPIPELINE_H
#define VULKANPROJECT_FONTPIPELINE_H

#include "../../GraphicsPipeline.h"

namespace vke {

  class FontPipeline final : public GraphicsPipeline {
  public:
    FontPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                 std::shared_ptr<RenderPass> renderPass,
                 VkDescriptorSetLayout fontDescriptorSetLayout);
  };
} // vke

#endif //VULKANPROJECT_FONTPIPELINE_H