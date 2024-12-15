#include "VulkanEngine.h"
#include <stdexcept>
#include <cstdint>

#include "utilities/Images.h"

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

VulkanEngine::VulkanEngine(VulkanEngineOptions vulkanEngineOptions)
  : vulkanEngineOptions(vulkanEngineOptions), currentFrame(0), framebufferResized(false)
{
  glfwInit();
  initVulkan();

  imGuiInstance = std::make_unique<ImGuiInstance>(commandPool, window, instance, physicalDevice, logicalDevice,
                                                  renderPass, guiPipeline);

  camera = std::make_shared<Camera>(vulkanEngineOptions.CAMERA_POSITION);
  camera->setSpeed(vulkanEngineOptions.CAMERA_SPEED);
}

VulkanEngine::~VulkanEngine()
{
  logicalDevice->waitIdle();

  vkDestroySampler(logicalDevice->getDevice(), sampler, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), descriptorSetLayout, nullptr);

  vkDestroyDescriptorPool(logicalDevice->getDevice(), descriptorPool, nullptr);

  vkDestroyCommandPool(logicalDevice->getDevice(), commandPool, nullptr);

  glfwTerminate();
}

bool VulkanEngine::isActive() const
{
  return window->isOpen();
}

void VulkanEngine::render()
{
  window->update();

  if (sceneFocused)
  {
    camera->processInput(window);
  }

  doComputing();
  doRendering();
}

std::shared_ptr<Texture> VulkanEngine::loadTexture(const char* path)
{
  auto texture = std::make_shared<Texture>(physicalDevice, logicalDevice, commandPool, path);
  textures.push_back(texture);

  return texture;
}

std::shared_ptr<Model> VulkanEngine::loadModel(const char* path, glm::vec3 rotation)
{
  auto model = std::make_shared<Model>(physicalDevice, logicalDevice, commandPool, path, rotation);
  models.push_back(model);

  return model;
}

std::shared_ptr<RenderObject> VulkanEngine::loadRenderObject(const std::shared_ptr<Texture>& texture,
                                                             const std::shared_ptr<Texture>& specularMap,
                                                             const std::shared_ptr<Model>& model) const
{
  auto renderObject = std::make_shared<RenderObject>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                                     objectsPipeline->getLayout(), texture, specularMap, model);

  objectsPipeline->insertRenderObject(renderObject);

  return renderObject;
}

void VulkanEngine::createLight(const glm::vec3 position, const glm::vec3 color, const float ambient,
                               const float diffuse, const float specular) const
{
  objectsPipeline->createLight(position, color, ambient, diffuse, specular);
}

ImGuiContext* VulkanEngine::getImGuiContext()
{
  return ImGui::GetCurrentContext();
}

bool VulkanEngine::keyIsPressed(const int key) const
{
  return window->keyIsPressed(key);
}

void VulkanEngine::initVulkan()
{
  instance = std::make_unique<Instance>();

  if (enableValidationLayers)
  {
    debugMessenger = std::make_unique<DebugMessenger>(instance->getInstance());
  }

  window = std::make_shared<Window>(vulkanEngineOptions.WINDOW_WIDTH, vulkanEngineOptions.WINDOW_HEIGHT,
                                    vulkanEngineOptions.WINDOW_TITLE, instance->getInstance(),
                                    vulkanEngineOptions.FULLSCREEN);

  physicalDevice = std::make_shared<PhysicalDevice>(instance->getInstance(), window->getSurface());

  logicalDevice = std::make_shared<LogicalDevice>(physicalDevice);

  createCommandPool();
  createOffscreenCommandBuffers();
  createCommandBuffers();
  createComputeCommandBuffers();

  swapChain = std::make_shared<SwapChain>(physicalDevice, logicalDevice, window);

  renderPass = std::make_shared<RenderPass>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                            swapChain->getImageFormat(), physicalDevice->getMsaaSamples(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  offscreenRenderPass = std::make_shared<RenderPass>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                            VK_FORMAT_B8G8R8A8_UNORM, physicalDevice->getMsaaSamples(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  framebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, swapChain, commandPool, renderPass);
  offscreenFramebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, swapChain, commandPool, renderPass, false);


  objectsPipeline = std::make_unique<ObjectsPipeline>(physicalDevice, logicalDevice, renderPass);

  guiPipeline = std::make_unique<GuiPipeline>(physicalDevice, logicalDevice, renderPass);

  if (vulkanEngineOptions.DO_DOTS)
  {
    dotsPipeline = std::make_unique<DotsPipeline>(physicalDevice, logicalDevice, commandPool,
                                                  renderPass->getRenderPass(), swapChain->getExtent());
  }


  // Create Sampler
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(logicalDevice->getDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create swapchain image sampler!");
  }

  // Create Descriptor Pool
  std::vector<VkDescriptorPoolSize> poolSizes = {
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(offscreenFramebuffer->framebufferImages.size())},
  };

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(offscreenFramebuffer->framebufferImages.size());

  if (vkCreateDescriptorPool(logicalDevice->getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool!");
  }

  // Create Descriptor Set Layout
  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &samplerLayoutBinding;

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }

  // Create Descriptor Sets
  framebufferDescriptorSets.resize(offscreenFramebuffer->framebufferImages.size());
  std::vector<VkDescriptorSetLayout> layouts(offscreenFramebuffer->framebufferImages.size(), descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(offscreenFramebuffer->framebufferImages.size());
  allocInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(logicalDevice->getDevice(), &allocInfo, framebufferDescriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor sets!");
  }

  // Update Descriptor Sets
  for (size_t i = 0; i < offscreenFramebuffer->framebufferImages.size(); i++) {
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = offscreenFramebuffer->framebufferImageViews[i];
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = framebufferDescriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(logicalDevice->getDevice(), 1, &descriptorWrite, 0, nullptr);
  }
}

void VulkanEngine::createCommandPool()
{
  auto queueFamilyIndices = physicalDevice->getQueueFamilies();

  const VkCommandPoolCreateInfo poolInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
  };

  if (vkCreateCommandPool(logicalDevice->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create command pool!");
  }
}

void VulkanEngine::createOffscreenCommandBuffers()
{
  offscreenCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  const VkCommandBufferAllocateInfo allocInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = static_cast<uint32_t>(offscreenCommandBuffers.size())
  };

  if (vkAllocateCommandBuffers(logicalDevice->getDevice(), &allocInfo, offscreenCommandBuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void VulkanEngine::createCommandBuffers()
{
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  const VkCommandBufferAllocateInfo allocInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())
  };

  if (vkAllocateCommandBuffers(logicalDevice->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void VulkanEngine::createComputeCommandBuffers()
{
  computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  const VkCommandBufferAllocateInfo allocInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = static_cast<uint32_t>(computeCommandBuffers.size())
  };

  if (vkAllocateCommandBuffers(logicalDevice->getDevice(), &allocInfo, computeCommandBuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void VulkanEngine::recordComputeCommandBuffer(const VkCommandBuffer& commandBuffer) const
{
  constexpr VkCommandBufferBeginInfo beginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
  };

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  if (vulkanEngineOptions.DO_DOTS)
  {
    dotsPipeline->compute(commandBuffer, currentFrame);
  }

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void VulkanEngine::recordOffscreenCommandBuffer(const VkCommandBuffer& commandBuffer, const uint32_t imageIndex) const
{
  constexpr VkCommandBufferBeginInfo beginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
  };

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  offscreenRenderPass->begin(offscreenFramebuffer->getFramebuffer(imageIndex), swapChain->getExtent(), commandBuffer);

  objectsPipeline->render(commandBuffer, currentFrame, camera, swapChain->getExtent());

  if (vulkanEngineOptions.DO_DOTS)
  {
    dotsPipeline->render(commandBuffer, currentFrame, swapChain->getExtent());
  }

  RenderPass::end(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void VulkanEngine::recordCommandBuffer(const VkCommandBuffer& commandBuffer, const uint32_t imageIndex) const
{
  constexpr VkCommandBufferBeginInfo beginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
  };

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  renderPass->begin(framebuffer->getFramebuffer(imageIndex), swapChain->getExtent(), commandBuffer);

  guiPipeline->render(commandBuffer, swapChain->getExtent());

  RenderPass::end(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void VulkanEngine::doComputing() const
{
  logicalDevice->waitForComputeFences(currentFrame);

  logicalDevice->resetComputeFences(currentFrame);

  vkResetCommandBuffer(computeCommandBuffers[currentFrame], 0);
  recordComputeCommandBuffer(computeCommandBuffers[currentFrame]);

  logicalDevice->submitComputeQueue(currentFrame, &computeCommandBuffers[currentFrame]);
}

void VulkanEngine::doRendering()
{
  sceneFocused = false;
  ImGui::Begin("Scene");

  if (ImGui::IsWindowFocused()) {
    sceneFocused = true;
  }

  // Render the image with the adjusted size
  ImGui::Image(reinterpret_cast<ImTextureID>(framebufferDescriptorSets[currentFrame]), ImGui::GetContentRegionAvail());

  ImGui::End();

  logicalDevice->waitForGraphicsFences(currentFrame);

  uint32_t imageIndex;
  auto result = logicalDevice->acquireNextImage(currentFrame, swapChain->getSwapChain(), &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    framebufferResized = false;
    recreateSwapChain();
    return;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  logicalDevice->resetGraphicsFences(currentFrame);

  //

  vkResetCommandBuffer(offscreenCommandBuffers[currentFrame], 0);
  recordOffscreenCommandBuffer(offscreenCommandBuffers[currentFrame], imageIndex);
  logicalDevice->submitOffscreenGraphicsQueue(currentFrame, &offscreenCommandBuffers[currentFrame]);

  //

  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  logicalDevice->submitGraphicsQueue(currentFrame, &commandBuffers[currentFrame]);

  result = logicalDevice->queuePresent(currentFrame, swapChain->getSwapChain(), &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
  {
    framebufferResized = false;
    recreateSwapChain();
  }
  else if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanEngine::recreateSwapChain()
{
  int width = 0, height = 0;
  window->getFramebufferSize(&width, &height);
  while (width == 0 || height == 0)
  {
    window->getFramebufferSize(&width, &height);
    glfwWaitEvents();
  }

  logicalDevice->waitIdle();

  framebuffer.reset();
  swapChain.reset();

  swapChain = std::make_shared<SwapChain>(physicalDevice, logicalDevice, window);
  framebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, swapChain, commandPool, renderPass);
  offscreenFramebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, swapChain, commandPool, renderPass, false);

  // Update Descriptor Sets
  for (size_t i = 0; i < offscreenFramebuffer->framebufferImages.size(); i++) {
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = offscreenFramebuffer->framebufferImageViews[i];
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = framebufferDescriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(logicalDevice->getDevice(), 1, &descriptorWrite, 0, nullptr);
  }
}
