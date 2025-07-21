#ifndef DESCRIPTORSET_H
#define DESCRIPTORSET_H

#include <vulkan/vulkan.h>
#include <functional>
#include <memory>
#include <vector>

class LogicalDevice;

class DescriptorSet {
public:
  DescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice, VkDescriptorPool descriptorPool);

  virtual ~DescriptorSet();

  void updateDescriptorSets(const std::function<std::vector<VkWriteDescriptorSet>(VkDescriptorSet descriptorSet, size_t frame)>& getWriteDescriptorSets) const;

  [[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const;

  [[nodiscard]] VkDescriptorSet& getDescriptorSet(size_t frame);

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_descriptorSets;

  void createDescriptorSet();

  void createDescriptorSetLayout();

  void allocateDescriptorSets();

  [[nodiscard]] virtual std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings() = 0;
};



#endif //DESCRIPTORSET_H
