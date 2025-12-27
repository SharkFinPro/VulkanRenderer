#ifndef VKE_DESCRIPTORSET_H
#define VKE_DESCRIPTORSET_H

#include <vulkan/vulkan.h>
#include <functional>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  class DescriptorSet final {
  public:
    explicit DescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           VkDescriptorPool descriptorPool,
                           const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings);

    explicit DescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           VkDescriptorPool descriptorPool,
                           VkDescriptorSetLayout descriptorSetLayout);

    ~DescriptorSet();

    void updateDescriptorSets(const std::function<std::vector<VkWriteDescriptorSet>(VkDescriptorSet descriptorSet, size_t frame)>& getWriteDescriptorSets) const;

    [[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const;

    [[nodiscard]] VkDescriptorSet& getDescriptorSet(size_t frame);

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    bool m_ownsLayout = false;

    std::vector<VkDescriptorSet> m_descriptorSets;

    void createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings);

    void allocateDescriptorSets(VkDescriptorPool descriptorPool);
  };

} // namespace vke

#endif //VKE_DESCRIPTORSET_H
