#ifndef MOUSEPICKINGPIPELINE_H
#define MOUSEPICKINGPIPELINE_H

#include "../GraphicsPipeline.h"

class RenderPass;

class MousePickingPipeline final : public GraphicsPipeline {
public:
  MousePickingPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                       const std::shared_ptr<RenderPass>& renderPass,
                       VkDescriptorSetLayout objectDescriptorSetLayout);

  void render(const RenderInfo* renderInfo, const std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>>* objects);

private:
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  void loadGraphicsDescriptorSetLayouts() override;
};



#endif //MOUSEPICKINGPIPELINE_H
