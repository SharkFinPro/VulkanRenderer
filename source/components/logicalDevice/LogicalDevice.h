#ifndef VKE_LOGICALDEVICE_H
#define VKE_LOGICALDEVICE_H

#include <vulkan/vulkan_raii.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace vke {

  class PhysicalDevice;

  class LogicalDevice {
  public:
    explicit LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);

    [[nodiscard]] std::shared_ptr<PhysicalDevice> getPhysicalDevice() const;

    void waitIdle() const;

    [[nodiscard]] vk::raii::Queue getGraphicsQueue() const;
    [[nodiscard]] vk::raii::Queue getPresentQueue() const;
    [[nodiscard]] vk::raii::Queue getComputeQueue() const;

    void submitMousePickingGraphicsQueue(uint32_t currentFrame,
                                         const vk::raii::CommandBuffer* commandBuffer) const;

    void submitOffscreenGraphicsQueue(uint32_t currentFrame,
                                      const vk::raii::CommandBuffer* commandBuffer) const;

    void submitGraphicsQueue(uint32_t currentFrame,
                             const vk::raii::CommandBuffer* commandBuffer) const;

    void submitComputeQueue(uint32_t currentFrame,
                            const vk::raii::CommandBuffer* commandBuffer) const;

    void waitForGraphicsFences(uint32_t currentFrame) const;
    void waitForComputeFences(uint32_t currentFrame) const;
    void waitForMousePickingFences(uint32_t currentFrame) const;

    void resetGraphicsFences(uint32_t currentFrame) const;
    void resetMousePickingFences(uint32_t currentFrame) const;
    void resetComputeFences(uint32_t currentFrame) const;

    vk::Result queuePresent(uint32_t currentFrame,
                          const vk::raii::SwapchainKHR& swapchain,
                          const uint32_t* imageIndex) const;

    void acquireNextImage(uint32_t currentFrame,
                          const vk::raii::SwapchainKHR& swapchain,
                          uint32_t* imageIndex) const;

    [[nodiscard]] uint32_t getMaxFramesInFlight() const;

    [[nodiscard]] vk::raii::CommandPool createCommandPool(const vk::CommandPoolCreateInfo& commandPoolCreateInfo) const;

    void allocateCommandBuffers(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo,
                                vk::CommandBuffer* commandBuffers) const;

    [[nodiscard]] vk::raii::DescriptorPool createDescriptorPool(const vk::DescriptorPoolCreateInfo& descriptorPoolCreateInfo) const;

    [[nodiscard]] vk::raii::DescriptorSetLayout createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo) const;

    static void doMappedMemoryOperation(const vk::raii::DeviceMemory& deviceMemory,
                                        const std::function<void(void* data)>& operationFunction);

    static void mapMemory(const vk::raii::DeviceMemory& memory,
                          vk::DeviceSize offset,
                          vk::DeviceSize size,
                          vk::MemoryMapFlags flags,
                          void** data);

    static void unmapMemory(const vk::raii::DeviceMemory& memory);

    void allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& descriptorSetAllocateInfo,
                                vk::raii::DescriptorSet* descriptorSets) const;

    void updateDescriptorSets(const vk::WriteDescriptorSet& descriptorWrites) const;

    [[nodiscard]] vk::raii::Buffer createBuffer(const vk::BufferCreateInfo& bufferCreateInfo) const;

    [[nodiscard]] static vk::MemoryRequirements getBufferMemoryRequirements(const vk::raii::Buffer& buffer);

    void allocateMemory(const vk::MemoryAllocateInfo& memoryAllocateInfo,
                        vk::raii::DeviceMemory& deviceMemory) const;

    static void bindBufferMemory(const vk::raii::Buffer& buffer,
                                 const vk::raii::DeviceMemory& deviceMemory,
                                 vk::DeviceSize memoryOffset = 0);

    [[nodiscard]] vk::raii::Sampler createSampler(const vk::SamplerCreateInfo& samplerCreateInfo) const;

    [[nodiscard]] vk::raii::ImageView createImageView(const vk::ImageViewCreateInfo& imageViewCreateInfo) const;

    [[nodiscard]] vk::raii::Image createImage(const vk::ImageCreateInfo& imageCreateInfo) const;

    [[nodiscard]] static vk::MemoryRequirements getImageMemoryRequirements(const vk::raii::Image& image);

    static void bindImageMemory(const vk::raii::Image& image,
                                const vk::raii::DeviceMemory& deviceMemory,
                                vk::DeviceSize memoryOffset = 0);

    [[nodiscard]] vk::raii::RenderPass createRenderPass(const vk::RenderPassCreateInfo& renderPassCreateInfo) const;

    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const vk::ShaderModuleCreateInfo& shaderModuleCreateInfo) const;

    [[nodiscard]] vk::raii::SwapchainKHR createSwapchain(const vk::SwapchainCreateInfoKHR& swapchainCreateInfo) const;

    static void getSwapchainImagesKHR(const vk::raii::SwapchainKHR& swapchain,
                                      uint32_t* swapchainImageCount,
                                      vk::Image* swapchainImages);

    [[nodiscard]] vk::raii::Framebuffer createFramebuffer(const vk::FramebufferCreateInfo& framebufferCreateInfo) const;

    [[nodiscard]] vk::raii::PipelineLayout createPipelineLayout(const vk::PipelineLayoutCreateInfo& pipelineLayoutCreateInfo) const;

    [[nodiscard]] vk::raii::Pipeline createPipeline(const vk::GraphicsPipelineCreateInfo& graphicsPipelineCreateInfo) const;

    [[nodiscard]] vk::raii::Pipeline createPipeline(const vk::ComputePipelineCreateInfo& computePipelineCreateInfo) const;

    [[nodiscard]] vk::raii::Pipeline createPipeline(const vk::RayTracingPipelineCreateInfoKHR& rayTracingPipelineCreateInfo) const;

    [[nodiscard]] vk::DeviceAddress getBufferDeviceAddress(const vk::raii::Buffer& buffer) const;

    void createAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& accelerationStructureCreateInfo,
                                     vk::raii::AccelerationStructureKHR* accelerationStructure) const;

    void getAccelerationStructureBuildSizes(const vk::AccelerationStructureBuildGeometryInfoKHR& accelerationStructureBuildGeometryInfo,
                                            uint32_t maxPrimitiveCounts,
                                            vk::AccelerationStructureBuildSizesInfoKHR* accelerationStructureBuildSizesInfo) const;

    static void buildAccelerationStructures(const vk::raii::CommandBuffer& commandBuffer,
                                            const vk::AccelerationStructureBuildGeometryInfoKHR& pInfos,
                                            const vk::AccelerationStructureBuildRangeInfoKHR* ppBuildRangeInfos);

    [[nodiscard]] vk::DeviceAddress getAccelerationStructureDeviceAddress(const vk::AccelerationStructureDeviceAddressInfoKHR* accelerationStructureDeviceAddressInfo) const;

    static void getRayTracingShaderGroupHandles(const vk::raii::Pipeline& pipeline,
                                                uint32_t groupCount,
                                                std::vector<uint8_t>& handles);

    static void traceRays(const vk::raii::CommandBuffer& commandBuffer,
                          const vk::StridedDeviceAddressRegionKHR& pRaygenShaderBindingTable,
                          const vk::StridedDeviceAddressRegionKHR& pMissShaderBindingTable,
                          const vk::StridedDeviceAddressRegionKHR& pHitShaderBindingTable,
                          const vk::StridedDeviceAddressRegionKHR& pCallableShaderBindingTable,
                          uint32_t width,
                          uint32_t height,
                          uint32_t depth);

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
