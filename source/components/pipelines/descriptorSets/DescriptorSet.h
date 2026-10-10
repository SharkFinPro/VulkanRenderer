#ifndef VKE_DESCRIPTORSET_H
#define VKE_DESCRIPTORSET_H

#include <vulkan/vulkan_raii.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace vke {

  class DescriptorAllocator;
  class LogicalDevice;

  class DescriptorSet final {
  public:
    explicit DescriptorSet(std::shared_ptr<LogicalDevice> logicalDevice,
                           vk::DescriptorPool descriptorPool,
                           const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings,
                           const void* allocationPNext = nullptr);

    explicit DescriptorSet(std::shared_ptr<LogicalDevice> logicalDevice,
                           vk::DescriptorPool descriptorPool,
                           vk::DescriptorSetLayout descriptorSetLayout,
                           const void* allocationPNext = nullptr);

    // The sets are returned to the allocator once no frame in flight can use them.
    explicit DescriptorSet(std::shared_ptr<LogicalDevice> logicalDevice,
                           std::shared_ptr<DescriptorAllocator> descriptorAllocator,
                           vk::DescriptorSetLayout descriptorSetLayout,
                           const void* allocationPNext = nullptr);

    ~DescriptorSet();

    DescriptorSet(const DescriptorSet&) = delete;
    DescriptorSet& operator=(const DescriptorSet&) = delete;

    void updateDescriptorSets(const std::function<std::vector<vk::WriteDescriptorSet>(vk::DescriptorSet descriptorSet, size_t frame)>& getWriteDescriptorSets) const;

    [[nodiscard]] vk::DescriptorSetLayout getDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSet getDescriptorSet(size_t frame) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::DescriptorSetLayout m_descriptorSetLayoutRAII = nullptr;
    vk::DescriptorSetLayout m_descriptorSetLayout = nullptr;

    std::vector<vk::DescriptorSet> m_descriptorSets;

    std::shared_ptr<DescriptorAllocator> m_descriptorAllocator;
    vk::DescriptorPool m_allocatorPool = nullptr;

    void createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& layoutBindings);

    void allocateDescriptorSets(vk::DescriptorPool descriptorPool,
                                const void* allocationPNext);
  };

} // namespace vke

#endif //VKE_DESCRIPTORSET_H
