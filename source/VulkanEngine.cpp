#include "VulkanEngine.h"
#include <stdexcept>
#include <cstdint>

#include "utilities/Images.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VulkanEngine::VulkanEngine(VulkanEngineOptions vulkanEngineOptions)
  : vulkanEngineOptions(vulkanEngineOptions)
{
  glfwInit();
  initVulkan();

  imGuiInstance = std::make_unique<ImGuiInstance>(commandPool, window, instance, physicalDevice, logicalDevice,
                                                  renderPass, guiPipeline);

  camera = std::make_shared<Camera>(vulkanEngineOptions.cameraPosition);
  camera->setSpeed(vulkanEngineOptions.cameraSpeed);
}

VulkanEngine::~VulkanEngine()
{
  logicalDevice->waitIdle();

  vkDestroyCommandPool(logicalDevice->getDevice(), commandPool, nullptr);

  cleanupSwapChain();

  glfwTerminate();
}

bool VulkanEngine::isActive() const
{
  return window->isOpen();
}

void VulkanEngine::render()
{
  window->update();

  camera->processInput(window);

  drawFrame();
}

std::shared_ptr<Texture> VulkanEngine::loadTexture(const char* path)
{
  auto texture = std::make_shared<Texture>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                           commandPool, logicalDevice->getGraphicsQueue(), path);
  textures.push_back(texture);

  return texture;
}

std::shared_ptr<Model> VulkanEngine::loadModel(const char* path)
{
  auto model = std::make_shared<Model>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                       commandPool, logicalDevice->getGraphicsQueue(), path);
  models.push_back(model);

  return model;
}

std::shared_ptr<RenderObject> VulkanEngine::loadRenderObject(const std::shared_ptr<Texture>& texture,
                                                             const std::shared_ptr<Texture>& specularMap,
                                                             const std::shared_ptr<Model>& model)
{
  auto renderObject = std::make_shared<RenderObject>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                                     graphicsPipeline->getLayout(), texture, specularMap, model);

  graphicsPipeline->insertRenderObject(renderObject);

  return renderObject;
}

void VulkanEngine::initVulkan()
{
  instance = std::make_unique<Instance>();

  if (enableValidationLayers)
  {
    debugMessenger = std::make_unique<DebugMessenger>(instance->getInstance());
  }

  window = std::make_shared<Window>(vulkanEngineOptions.WINDOW_WIDTH, vulkanEngineOptions.WINDOW_HEIGHT,
                                    vulkanEngineOptions.WINDOW_TITLE, instance->getInstance());

  physicalDevice = std::make_shared<PhysicalDevice>(instance->getInstance(), window->getSurface());

  logicalDevice = std::make_unique<LogicalDevice>(physicalDevice);

  swapChain = std::make_shared<SwapChain>(physicalDevice, logicalDevice, window);

  renderPass = std::make_shared<RenderPass>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), swapChain->getImageFormat(),
                                            physicalDevice->getMsaaSamples(), findDepthFormat());

  graphicsPipeline = std::make_unique<GraphicsPipeline>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                                        vulkanEngineOptions.VERTEX_SHADER_FILE,
                                                        vulkanEngineOptions.FRAGMENT_SHADER_FILE,
                                                        swapChain->getExtent(), physicalDevice->getMsaaSamples(),renderPass);

  guiPipeline = std::make_unique<GuiPipeline>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                              "assets/shaders/ui_vert.spv",
                                              "assets/shaders/ui_frag.spv",
                                              swapChain->getExtent(), physicalDevice->getMsaaSamples(), renderPass);

  createCommandPool();
  createColorResources();
  createDepthResources();
  createFrameBuffers();
  createCommandBuffers();
}

void VulkanEngine::createFrameBuffers()
{
  auto swapChainImageViews = swapChain->getImageViews();

  swapChainFramebuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++)
  {
    std::array<VkImageView, 3> attachments = {
      colorImageView,
      depthImageView,
      swapChainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass->getRenderPass();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChain->getExtent().width;
    framebufferInfo.height = swapChain->getExtent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(logicalDevice->getDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void VulkanEngine::createCommandPool()
{
  auto queueFamilyIndices = physicalDevice->getQueueFamilies();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(logicalDevice->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create command pool!");
  }
}

void VulkanEngine::createCommandBuffers()
{
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

  if (vkAllocateCommandBuffers(logicalDevice->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass->getRenderPass();
  renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChain->getExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  graphicsPipeline->render(commandBuffer, currentFrame, camera);

  guiPipeline->render(commandBuffer);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void VulkanEngine::drawFrame()
{
  logicalDevice->waitForFences(currentFrame);

  uint32_t imageIndex;
  auto result = logicalDevice->acquireNextImage(currentFrame, swapChain->getSwapChain(), &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    framebufferResized = false;
    recreateSwapChain();
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  logicalDevice->resetFences(currentFrame);

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

void VulkanEngine::cleanupSwapChain()
{
  vkDestroyImageView(logicalDevice->getDevice(), colorImageView, nullptr);
  vkDestroyImage(logicalDevice->getDevice(), colorImage, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), colorImageMemory, nullptr);

  vkDestroyImageView(logicalDevice->getDevice(), depthImageView, nullptr);
  vkDestroyImage(logicalDevice->getDevice(), depthImage, nullptr);
  vkFreeMemory(logicalDevice->getDevice(), depthImageMemory, nullptr);

  for (auto framebuffer : swapChainFramebuffers)
  {
    vkDestroyFramebuffer(logicalDevice->getDevice(), framebuffer, nullptr);
  }

  swapChain.reset();
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

  cleanupSwapChain();

  swapChain = std::make_shared<SwapChain>(physicalDevice, logicalDevice, window);

  createColorResources();
  createDepthResources();
  createFrameBuffers();
}

void VulkanEngine::createDepthResources()
{
  VkFormat depthFormat = findDepthFormat();

  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), swapChain->getExtent().width, swapChain->getExtent().height,
                      1, physicalDevice->getMsaaSamples(), depthFormat, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              depthImage, depthImageMemory);
  depthImageView = Images::createImageView(logicalDevice->getDevice(), depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  Images::transitionImageLayout(logicalDevice->getDevice(), commandPool, logicalDevice->getGraphicsQueue(),
                                depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

VkFormat VulkanEngine::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                           VkFormatFeatureFlags features)
{
  for (auto format : candidates)
  {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice->getPhysicalDevice(), format, &props);

    if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) ||
        (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features))
    {
      return format;
    }
  }

  throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanEngine::findDepthFormat()
{
  return findSupportedFormat(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

void VulkanEngine::createColorResources()
{
  VkFormat colorFormat = swapChain->getImageFormat();

  Images::createImage(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), swapChain->getExtent().width, swapChain->getExtent().height,
                      1, physicalDevice->getMsaaSamples(), colorFormat, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);

  colorImageView = Images::createImageView(logicalDevice->getDevice(), colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}
