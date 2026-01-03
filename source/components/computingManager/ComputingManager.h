#ifndef VKE_COMPUTINGMANAGER_H
#define VKE_COMPUTINGMANAGER_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class CommandBuffer;
  class LogicalDevice;
  class PipelineManager;
  class Renderer2D;
  class Renderer3D;

  class ComputingManager {
  public:
    explicit ComputingManager(std::shared_ptr<LogicalDevice> logicalDevice);

    ~ComputingManager();

    void doComputing(const std::shared_ptr<PipelineManager>& pipelineManager,
                     uint32_t currentFrame,
                     const std::shared_ptr<Renderer2D>& renderer2D,
                     const std::shared_ptr<Renderer3D>& renderer3D) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    std::shared_ptr<CommandBuffer> m_computeCommandBuffer;

    void recordComputeCommandBuffer(const std::shared_ptr<PipelineManager>& pipelineManager,
                                    uint32_t currentFrame,
                                    const std::shared_ptr<Renderer2D>& renderer2D,
                                    const std::shared_ptr<Renderer3D>& renderer3D) const;

    void createCommandPool();
  };

} // namespace vke

#endif //VKE_COMPUTINGMANAGER_H
