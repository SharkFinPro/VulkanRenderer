#ifndef VKE_PIPELINE_H
#define VKE_PIPELINE_H

#include "../commandBuffer/CommandBuffer.h"
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

    template<typename T>
    void pushConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                       vk::ShaderStageFlags stageFlags,
                       uint32_t offset,
                       const T& data) const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::PipelineLayout m_pipelineLayout = nullptr;

    vk::raii::Pipeline m_pipeline = nullptr;
  };

  template<typename T>
  void Pipeline::pushConstants(const std::shared_ptr<CommandBuffer>& commandBuffer,
                               const vk::ShaderStageFlags stageFlags,
                               const uint32_t offset,
                               const T& data) const
  {
    commandBuffer->pushConstants<T>(m_pipelineLayout, stageFlags, offset, data);
  }
} // namespace vke

#endif //VKE_PIPELINE_H