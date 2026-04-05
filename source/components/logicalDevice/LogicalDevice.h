#ifndef VKE_LOGICALDEVICE_H
#define VKE_LOGICALDEVICE_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class PhysicalDevice;

  class LogicalDevice {
  public:
    explicit LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);

    [[nodiscard]] std::shared_ptr<PhysicalDevice> getPhysicalDevice() const;

    void waitIdle() const;

    [[nodiscard]] vk::Queue getGraphicsQueue() const;
    [[nodiscard]] vk::Queue getPresentQueue() const;
    [[nodiscard]] vk::Queue getComputeQueue() const;

    void submitMousePickingGraphicsQueue(uint32_t currentFrame,
                                         vk::CommandBuffer commandBuffer) const;

    void submitOffscreenGraphicsQueue(uint32_t currentFrame,
                                      vk::CommandBuffer commandBuffer) const;

    void submitGraphicsQueue(uint32_t currentFrame,
                             vk::CommandBuffer commandBuffer) const;

    void submitComputeQueue(uint32_t currentFrame,
                            vk::CommandBuffer commandBuffer) const;

    void waitForGraphicsFences(uint32_t currentFrame) const;
    void waitForComputeFences(uint32_t currentFrame) const;
    void waitForMousePickingFences(uint32_t currentFrame) const;

    void resetGraphicsFences(uint32_t currentFrame) const;
    void resetMousePickingFences(uint32_t currentFrame) const;
    void resetComputeFences(uint32_t currentFrame) const;

    vk::Result queuePresent(uint32_t currentFrame,
                            vk::SwapchainKHR swapchain,
                            const uint32_t* imageIndex) const;

    vk::Result acquireNextImage(uint32_t currentFrame,
                                vk::SwapchainKHR swapchain,
                                uint32_t* imageIndex) const;

    [[nodiscard]] uint32_t getMaxFramesInFlight() const;

    [[nodiscard]] vk::raii::CommandPool createCommandPool(const vk::CommandPoolCreateInfo& commandPoolCreateInfo) const;

    void allocateCommandBuffers(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo,
                                std::vector<vk::raii::CommandBuffer>& commandBuffers) const;

    [[nodiscard]] vk::raii::DescriptorPool createDescriptorPool(const vk::DescriptorPoolCreateInfo& descriptorPoolCreateInfo) const;

    [[nodiscard]] vk::raii::DescriptorSetLayout createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo) const;

    [[nodiscard]] std::vector<vk::raii::DescriptorSet> allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& descriptorSetAllocateInfo) const;

    void updateDescriptorSets(const std::vector<vk::WriteDescriptorSet>& writeDescriptorSets) const;

    [[nodiscard]] vk::raii::Buffer createBuffer(const vk::BufferCreateInfo& bufferCreateInfo) const;

    void allocateMemory(const vk::MemoryAllocateInfo& memoryAllocateInfo,
                        vk::raii::DeviceMemory& deviceMemory) const;

    [[nodiscard]] vk::raii::Sampler createSampler(const vk::SamplerCreateInfo& samplerCreateInfo) const;

    [[nodiscard]] vk::raii::ImageView createImageView(const vk::ImageViewCreateInfo& imageViewCreateInfo) const;

    [[nodiscard]] vk::raii::Image createImage(const vk::ImageCreateInfo& imageCreateInfo) const;

    [[nodiscard]] vk::raii::RenderPass createRenderPass(const vk::RenderPassCreateInfo& renderPassCreateInfo) const;

    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const vk::ShaderModuleCreateInfo& shaderModuleCreateInfo) const;

    [[nodiscard]] vk::raii::SwapchainKHR createSwapchain(const vk::SwapchainCreateInfoKHR& swapchainCreateInfo) const;

    [[nodiscard]] vk::raii::Framebuffer createFramebuffer(const vk::FramebufferCreateInfo& framebufferCreateInfo) const;

    [[nodiscard]] vk::raii::PipelineLayout createPipelineLayout(const vk::PipelineLayoutCreateInfo& pipelineLayoutCreateInfo) const;

    [[nodiscard]] vk::raii::Pipeline createPipeline(const vk::GraphicsPipelineCreateInfo& graphicsPipelineCreateInfo) const;

    [[nodiscard]] vk::raii::Pipeline createPipeline(const vk::ComputePipelineCreateInfo& computePipelineCreateInfo) const;

    [[nodiscard]] vk::raii::Pipeline createPipeline(const vk::RayTracingPipelineCreateInfoKHR& rayTracingPipelineCreateInfo) const;

    [[nodiscard]] vk::DeviceAddress getBufferDeviceAddress(vk::Buffer buffer) const;

    [[nodiscard]] vk::raii::AccelerationStructureKHR createAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& accelerationStructureCreateInfo) const;

    void getAccelerationStructureBuildSizes(const vk::AccelerationStructureBuildGeometryInfoKHR& accelerationStructureBuildGeometryInfo,
                                            uint32_t maxPrimitiveCounts,
                                            vk::AccelerationStructureBuildSizesInfoKHR& accelerationStructureBuildSizesInfo) const;

    [[nodiscard]] vk::DeviceAddress getAccelerationStructureDeviceAddress(const vk::AccelerationStructureDeviceAddressInfoKHR* accelerationStructureDeviceAddressInfo) const;

    friend class ImGuiInstance;

  private:
    std::shared_ptr<PhysicalDevice> m_physicalDevice;

    vk::raii::Device m_device = nullptr;

    vk::raii::Queue m_graphicsQueue = nullptr;
    vk::raii::Queue m_presentQueue = nullptr;
    vk::raii::Queue m_computeQueue = nullptr;

    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;

    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores2;

    std::vector<vk::raii::Fence> m_inFlightFences;
    std::vector<vk::raii::Fence> m_offscreenInFlightFences;
    std::vector<vk::raii::Fence> m_mousePickingInFlightFences;

    std::vector<vk::raii::Semaphore> m_computeFinishedSemaphores;
    std::vector<vk::raii::Fence> m_computeInFlightFences;

    uint8_t m_maxFramesInFlight = 2;

    void createDevice();

    void createSyncObjects();
  };

} // namespace vke

#endif //VKE_LOGICALDEVICE_H
