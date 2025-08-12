#ifndef VULKANPROJECT_OBJECTSPIPELINE_H
#define VULKANPROJECT_OBJECTSPIPELINE_H

#include "../../GraphicsPipeline.h"
#include <memory>

class DescriptorSet;
class RenderPass;

class ObjectsPipeline final : public GraphicsPipeline {
public:
  ObjectsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  std::shared_ptr<RenderPass> renderPass,
                  VkDescriptorSetLayout objectDescriptorSetLayout,
                  const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

private:
  std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};


#endif //VULKANPROJECT_OBJECTSPIPELINE_H
