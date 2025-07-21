#ifndef DESCRIPTORSET_H
#define DESCRIPTORSET_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class LogicalDevice;

class DescriptorSet {
public:
  DescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice, VkDescriptorPool descriptorPool);

  virtual ~DescriptorSet();

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_descriptorSets;

  void createDescriptorSet();

  void createDescriptorSetLayout();

  void allocateDescriptorSets();

  void updateDescriptorSets();

  [[nodiscard]] virtual std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings() = 0;

  [[nodiscard]] virtual std::vector<VkWriteDescriptorSet> getWriteDescriptorSets(size_t frame) = 0;
};



#endif //DESCRIPTORSET_H
