#ifndef OBJECTHIGHLIGHTPIPELINE_H
#define OBJECTHIGHLIGHTPIPELINE_H

#include "../GraphicsPipeline.h"

class RenderPass;

class ObjectHighlightPipeline final : public GraphicsPipeline {
public:
  ObjectHighlightPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                          const std::shared_ptr<RenderPass>& renderPass,
                          VkDescriptorSetLayout objectDescriptorSetLayout);

  void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) override;

private:
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;
};



#endif //OBJECTHIGHLIGHTPIPELINE_H
