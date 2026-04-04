#ifndef VKE_DESCRIPTORSET_H
#define VKE_DESCRIPTORSET_H

#include <vulkan/vulkan_raii.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  class DescriptorSet final {
  public:
    explicit DescriptorSet(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::raii::DescriptorPool& descriptorPool,
                           const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings,
                           const void* allocationPNext = nullptr);

    explicit DescriptorSet(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::raii::DescriptorPool& descriptorPool,
                           vk::raii::DescriptorSetLayout descriptorSetLayout,
                           const void* allocationPNext = nullptr);

    void updateDescriptorSets(const std::function<std::vector<vk::WriteDescriptorSet>(vk::DescriptorSet descriptorSet, size_t frame)>& getWriteDescriptorSets) const;

    [[nodiscard]] vk::DescriptorSetLayout getDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSet getDescriptorSet(size_t frame) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::DescriptorSetLayout m_descriptorSetLayout = nullptr;
    bool m_ownsLayout = false;

    std::vector<vk::raii::DescriptorSet> m_descriptorSets;

    void createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings);

    void allocateDescriptorSets(const vk::raii::DescriptorPool& descriptorPool,
                                const void* allocationPNext);
  };

} // namespace vke

#endif //VKE_DESCRIPTORSET_H
