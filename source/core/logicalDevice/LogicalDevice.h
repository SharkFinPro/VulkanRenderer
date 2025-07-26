#ifndef VULKANPROJECT_LOGICALDEVICE_H
#define VULKANPROJECT_LOGICALDEVICE_H

#include <vulkan/vulkan.h>
#include <functional>
#include <memory>
#include <vector>

class PhysicalDevice;

class LogicalDevice {
public:
  explicit LogicalDevice(const std::shared_ptr<PhysicalDevice>& physicalDevice);
  ~LogicalDevice();

  [[nodiscard]] std::shared_ptr<PhysicalDevice> getPhysicalDevice() const;

  void waitIdle() const;

  [[nodiscard]] VkQueue getGraphicsQueue() const;
  [[nodiscard]] VkQueue getPresentQueue() const;
  [[nodiscard]] VkQueue getComputeQueue() const;

  void submitMousePickingGraphicsQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;
  void submitOffscreenGraphicsQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;
  void submitGraphicsQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;
  void submitComputeQueue(uint32_t currentFrame, const VkCommandBuffer* commandBuffer) const;

  void waitForGraphicsFences(uint32_t currentFrame) const;
  void waitForComputeFences(uint32_t currentFrame) const;
  void waitForMousePickingFences(uint32_t currentFrame) const;

  void resetGraphicsFences(uint32_t currentFrame) const;
  void resetMousePickingFences(uint32_t currentFrame) const;
  void resetComputeFences(uint32_t currentFrame) const;

  VkResult queuePresent(uint32_t currentFrame, const VkSwapchainKHR& swapchain, const uint32_t* imageIndex) const;

  VkResult acquireNextImage(uint32_t currentFrame, const VkSwapchainKHR& swapchain, uint32_t* imageIndex) const;

  [[nodiscard]] uint32_t getMaxFramesInFlight() const;

  [[nodiscard]] VkCommandPool createCommandPool(const VkCommandPoolCreateInfo& commandPoolCreateInfo) const;

  void destroyCommandPool(VkCommandPool& commandPool) const;

  void allocateCommandBuffers(const VkCommandBufferAllocateInfo& commandBufferAllocateInfo,
                              VkCommandBuffer* commandBuffers) const;

  void freeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* commandBuffers) const;

  [[nodiscard]] VkDescriptorPool createDescriptorPool(const VkDescriptorPoolCreateInfo& descriptorPoolCreateInfo) const;

  void destroyDescriptorPool(VkDescriptorPool& descriptorPool) const;

  [[nodiscard]] VkDescriptorSetLayout createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo) const;

  void destroyDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout) const;

  void doMappedMemoryOperation(VkDeviceMemory deviceMemory, const std::function<void(void* data)>& operationFunction) const;

  void mapMemory(const VkDeviceMemory& memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** data) const;

  void unmapMemory(const VkDeviceMemory& memory) const;

  void allocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo,
                              VkDescriptorSet* descriptorSets) const;

  void updateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet* descriptorWrites) const;

  [[nodiscard]] VkBuffer createBuffer(const VkBufferCreateInfo& bufferCreateInfo) const;

  void destroyBuffer(VkBuffer& buffer) const;

  [[nodiscard]] VkMemoryRequirements getBufferMemoryRequirements(const VkBuffer& buffer) const;

  void allocateMemory(const VkMemoryAllocateInfo& memoryAllocateInfo, VkDeviceMemory& deviceMemory) const;

  void freeMemory(VkDeviceMemory& memory) const;

  void bindBufferMemory(const VkBuffer& buffer, const VkDeviceMemory& deviceMemory, VkDeviceSize memoryOffset = 0) const;

  [[nodiscard]] VkSampler createSampler(const VkSamplerCreateInfo& samplerCreateInfo) const;

  void destroySampler(VkSampler& sampler) const;

  [[nodiscard]] VkImageView createImageView(const VkImageViewCreateInfo& imageViewCreateInfo) const;

  void destroyImageView(VkImageView& imageView) const;

  [[nodiscard]] VkImage createImage(const VkImageCreateInfo& imageCreateInfo) const;

  void destroyImage(VkImage& image) const;

  [[nodiscard]] VkMemoryRequirements getImageMemoryRequirements(const VkImage& image) const;

  void bindImageMemory(const VkImage& image, const VkDeviceMemory& deviceMemory, VkDeviceSize memoryOffset = 0) const;

  [[nodiscard]] VkRenderPass createRenderPass(const VkRenderPassCreateInfo& renderPassCreateInfo) const;

  void destroyRenderPass(VkRenderPass& renderPass) const;

  [[nodiscard]] VkShaderModule createShaderModule(const VkShaderModuleCreateInfo& shaderModuleCreateInfo) const;

  void destroyShaderModule(VkShaderModule& shaderModule) const;

  [[nodiscard]] VkSwapchainKHR createSwapchain(const VkSwapchainCreateInfoKHR& swapchainCreateInfo) const;

  void getSwapchainImagesKHR(const VkSwapchainKHR& swapchain, uint32_t* swapchainImageCount, VkImage* swapchainImages) const;

  void destroySwapchainKHR(VkSwapchainKHR& swapchain) const;

  [[nodiscard]] VkFramebuffer createFramebuffer(const VkFramebufferCreateInfo& framebufferCreateInfo) const;

  void destroyFramebuffer(VkFramebuffer& framebuffer) const;

  [[nodiscard]] VkPipelineLayout createPipelineLayout(const VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo) const;

  void destroyPipelineLayout(VkPipelineLayout& pipelineLayout) const;

  [[nodiscard]] VkPipeline createPipeline(const VkGraphicsPipelineCreateInfo& graphicsPipelineCreateInfo) const;

  [[nodiscard]] VkPipeline createPipeline(const VkComputePipelineCreateInfo& computePipelineCreateInfo) const;

  void destroyPipeline(VkPipeline& pipeline) const;

  friend class ImGuiInstance;

private:
  std::shared_ptr<PhysicalDevice> m_physicalDevice;

  VkDevice m_device = VK_NULL_HANDLE;

  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  VkQueue m_presentQueue = VK_NULL_HANDLE;
  VkQueue m_computeQueue = VK_NULL_HANDLE;

  std::vector<VkSemaphore> m_imageAvailableSemaphores;

  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores2;

  std::vector<VkFence> m_inFlightFences;
  std::vector<VkFence> m_inFlightFences2;
  std::vector<VkFence> m_mousePickingInFlightFences;

  std::vector<VkSemaphore> m_computeFinishedSemaphores;
  std::vector<VkFence> m_computeInFlightFences;

  uint8_t m_maxFramesInFlight = 2;

  void createDevice();

  void createSyncObjects();
};


#endif //VULKANPROJECT_LOGICALDEVICE_H
