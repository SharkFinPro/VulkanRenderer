#ifndef VKE_PIPELINE_H
#define VKE_PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;

  class Pipeline {
  public:
    explicit Pipeline(std::shared_ptr<LogicalDevice> logicalDevice);

    virtual ~Pipeline();

    virtual void displayGui() {}

    void pushConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                       VkShaderStageFlags stageFlags,
                       uint32_t offset,
                       uint32_t size,
                       const void* values) const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    VkPipeline m_pipeline = VK_NULL_HANDLE;
  };

} // namespace vke

#endif //VKE_PIPELINE_H
