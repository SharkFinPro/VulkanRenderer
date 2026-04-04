#ifndef VKE_PIPELINE_H
#define VKE_PIPELINE_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;

  class Pipeline {
  public:
    explicit Pipeline(std::shared_ptr<LogicalDevice> logicalDevice);

    virtual ~Pipeline() = default;

    virtual void displayGui() {}

    void pushConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                       vk::ShaderStageFlags stageFlags,
                       uint32_t offset,
                       uint32_t size,
                       const void* values) const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::PipelineLayout m_pipelineLayout = nullptr;

    vk::raii::Pipeline m_pipeline = nullptr;
  };

} // namespace vke

#endif //VKE_PIPELINE_H