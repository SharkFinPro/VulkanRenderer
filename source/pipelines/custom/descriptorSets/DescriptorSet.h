#ifndef DESCRIPTORSET_H
#define DESCRIPTORSET_H

#include <vulkan/vulkan.h>
#include <functional>
#include <memory>
#include <vector>

class LogicalDevice;

class DescriptorSet {
public:
  explicit DescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice);

  virtual ~DescriptorSet();

  void updateDescriptorSets(const std::function<std::vector<VkWriteDescriptorSet>(VkDescriptorSet descriptorSet, size_t frame)>& getWriteDescriptorSets) const;

  [[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const;

  [[nodiscard]] VkDescriptorSet& getDescriptorSet(size_t frame);

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_descriptorSets;

  void createDescriptorSet(VkDescriptorPool descriptorPool);

  void createDescriptorSetLayout();

  void allocateDescriptorSets(VkDescriptorPool descriptorPool);

  [[nodiscard]] virtual std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings() = 0;
};



#endif //DESCRIPTORSET_H
