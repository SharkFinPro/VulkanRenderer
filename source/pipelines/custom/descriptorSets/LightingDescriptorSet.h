#ifndef LIGHTINGDESCRIPTORSET_H
#define LIGHTINGDESCRIPTORSET_H

#include "DescriptorSet.h"

class LightingDescriptorSet final : public DescriptorSet {
public:
  LightingDescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice, VkDescriptorPool descriptorPool);

private:
  std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings() override;
};



#endif //LIGHTINGDESCRIPTORSET_H
