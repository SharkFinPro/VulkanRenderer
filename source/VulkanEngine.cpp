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

  camera = std::make_shared<Camera>(vulkanEngineOptions.cameraPosition);
  camera->setSpeed(vulkanEngineOptions.cameraSpeed);
}

VulkanEngine::~VulkanEngine()
{
  logicalDevice->waitIdle();

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
                                                             const std::shared_ptr<Model>& model) const
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

  logicalDevice = std::make_shared<LogicalDevice>(physicalDevice);

  swapChain = std::make_shared<SwapChain>(physicalDevice, logicalDevice, window);

  renderPass = std::make_shared<RenderPass>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(), swapChain->getImageFormat(),
                                            physicalDevice->getMsaaSamples());

  createCommandPool();
  framebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, swapChain, commandPool, renderPass);

  createCommandBuffers();
  createComputeCommandBuffers();

  graphicsPipeline = std::make_unique<GraphicsPipeline>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                                        "assets/shaders/vert.spv",
                                                        "assets/shaders/frag.spv",
                                                        swapChain->getExtent(), physicalDevice->getMsaaSamples(), renderPass);

  guiPipeline = std::make_unique<GuiPipeline>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                              "assets/shaders/ui_vert.spv",
                                              "assets/shaders/ui_frag.spv",
                                              swapChain->getExtent(), physicalDevice->getMsaaSamples(), renderPass);

  computePipeline = std::make_unique<ComputePipeline>(physicalDevice, logicalDevice, commandPool, renderPass->getRenderPass());
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
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(logicalDevice->getDevice(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void VulkanEngine::createComputeCommandBuffers()
{
  computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = static_cast<uint32_t>(computeCommandBuffers.size());

  if (vkAllocateCommandBuffers(logicalDevice->getDevice(), &allocInfo, computeCommandBuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void VulkanEngine::recordComputeCommandBuffer(VkCommandBuffer& commandBuffer) const
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  computePipeline->compute(commandBuffer, currentFrame);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer) const
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  renderPass->begin(framebuffer->getFramebuffer(currentFrame), swapChain->getExtent(), commandBuffer);

  graphicsPipeline->render(commandBuffer, currentFrame, camera);

  computePipeline->render(commandBuffer, currentFrame, swapChain->getExtent());

  guiPipeline->render(commandBuffer);

  RenderPass::end(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void VulkanEngine::drawFrame()
{
  // Compute
  logicalDevice->waitForComputeFences(currentFrame);

  computePipeline->updateUniformBuffer(currentFrame);

  logicalDevice->resetComputeFences(currentFrame);

  vkResetCommandBuffer(computeCommandBuffers[currentFrame], 0);
  recordComputeCommandBuffer(computeCommandBuffers[currentFrame]);

  logicalDevice->submitComputeQueue(currentFrame, &computeCommandBuffers[currentFrame]);

  // Graphics
  logicalDevice->waitForGraphicsFences(currentFrame);

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

  logicalDevice->resetGraphicsFences(currentFrame);

  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  recordCommandBuffer(commandBuffers[currentFrame]);

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
}
