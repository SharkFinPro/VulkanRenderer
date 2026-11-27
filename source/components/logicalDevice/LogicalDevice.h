#ifndef VKE_LOGICALDEVICE_H
#define VKE_LOGICALDEVICE_H

#include "PooledMemoryKey.h"
#include <vulkan/vulkan_raii.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

namespace vke {

  class DeferredDestructionQueue;
  class PhysicalDevice;

  class LogicalDevice {
  public:
    explicit LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);

    ~LogicalDevice();

    [[nodiscard]] std::shared_ptr<PhysicalDevice> getPhysicalDevice() const;

    void waitIdle() const;

    // Keeps a resource the GPU may still use alive until the last frame that could reference it has
    // completed, then destroys it. Never waits on the device.
    void retire(std::shared_ptr<void> resource);

    // Moves handles (raii objects, a vector of them, a struct of them) into the retire queue. Members of a
    // struct are destroyed in reverse declaration order.
    template <typename T>
    void retireHandles(T&& handles)
    {
      retire(std::make_shared<std::decay_t<T>>(std::forward<T>(handles)));
    }

    // Called once per frame by the frame scheduler.
    void beginFrameDestruction(uint64_t frameNumber,
                               uint64_t lastCompletedFrame);

    // Destroys everything retired so far. Requires an idle device.
    void destroyRetiredResources();

    [[nodiscard]] vk::Queue getGraphicsQueue() const;
    [[nodiscard]] vk::Queue getPresentQueue() const;
    [[nodiscard]] vk::Queue getComputeQueue() const;

    void submitToGraphicsQueue(const vk::SubmitInfo2& submitInfo) const;

    void submitToComputeQueue(const vk::SubmitInfo2& submitInfo) const;

    vk::Result queuePresent(const vk::PresentInfoKHR& presentInfo) const;

    [[nodiscard]] std::pair<vk::Result, uint32_t> acquireNextImage(const vk::AcquireNextImageInfoKHR& acquireInfo) const;

    [[nodiscard]] vk::raii::Semaphore createSemaphore(const vk::SemaphoreCreateInfo& semaphoreCreateInfo) const;

    void waitSemaphores(const vk::SemaphoreWaitInfo& waitInfo) const;

    void signalSemaphore(const vk::SemaphoreSignalInfo& signalInfo) const;

    [[nodiscard]] uint32_t getMaxFramesInFlight() const;

    [[nodiscard]] vk::raii::CommandPool createCommandPool(const vk::CommandPoolCreateInfo& commandPoolCreateInfo) const;

    void allocateCommandBuffers(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo,
                                std::vector<vk::raii::CommandBuffer>& commandBuffers) const;

    [[nodiscard]] vk::raii::DescriptorPool createDescriptorPool(const vk::DescriptorPoolCreateInfo& descriptorPoolCreateInfo) const;

    [[nodiscard]] vk::raii::DescriptorSetLayout createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo) const;

    [[nodiscard]] std::vector<vk::raii::DescriptorSet> allocateDescriptorSets(const vk::DescriptorSetAllocateInfo& descriptorSetAllocateInfo) const;

    void freeDescriptorSets(vk::DescriptorPool descriptorPool,
                            const std::vector<vk::DescriptorSet>& descriptorSets) const;

    void updateDescriptorSets(const vk::ArrayProxy<const vk::WriteDescriptorSet>& writeDescriptorSets) const;

    [[nodiscard]] vk::raii::Buffer createBuffer(const vk::BufferCreateInfo& bufferCreateInfo) const;

    void allocateMemory(const vk::MemoryAllocateInfo& memoryAllocateInfo,
                        vk::raii::DeviceMemory& deviceMemory) const;

    // Memory for a buffer whose memory is recycled instead of freed: the size is rounded up so blocks of similar
    // buffers are interchangeable, and a block handed back through retirePooledMemory is reused by the next request
    // with the same key. Skips the driver's allocation, which dominates creating many small meshes.
    void allocatePooledMemory(vk::DeviceSize requiredSize,
                              uint32_t memoryTypeIndex,
                              bool deviceAddress,
                              vk::raii::DeviceMemory& deviceMemory,
                              PooledMemoryKey& key);

    // Returns the block to the pool once the last frame that could use it has completed. Retire the buffers bound to
    // it first, so they are destroyed before it is handed out again.
    void retirePooledMemory(const PooledMemoryKey& key,
                            vk::raii::DeviceMemory&& deviceMemory);

    [[nodiscard]] vk::raii::Sampler createSampler(const vk::SamplerCreateInfo& samplerCreateInfo) const;

    [[nodiscard]] vk::raii::ImageView createImageView(const vk::ImageViewCreateInfo& imageViewCreateInfo) const;

    [[nodiscard]] vk::raii::Image createImage(const vk::ImageCreateInfo& imageCreateInfo) const;

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

    // Declared after m_device so retired resources are destroyed before the device.
    std::unique_ptr<DeferredDestructionQueue> m_retiredResources;

    // Declared after m_device so the blocks are freed before the device.
    std::mutex m_poolMutex;
    std::map<PooledMemoryKey, std::vector<vk::raii::DeviceMemory>> m_memoryPool;
    vk::DeviceSize m_pooledBytes = 0;

    void releaseToPool(const PooledMemoryKey& key,
                       vk::raii::DeviceMemory&& deviceMemory);

    vk::raii::Queue m_graphicsQueue = nullptr;
    vk::raii::Queue m_presentQueue = nullptr;
    vk::raii::Queue m_computeQueue = nullptr;

    uint8_t m_maxFramesInFlight = 2;

    void createDevice();
  };

} // namespace vke

#endif //VKE_LOGICALDEVICE_H
