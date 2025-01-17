#include "VulkanEngine.h"
#include <stdexcept>
#include <cstdint>
#include <backends/imgui_impl_vulkan.h>

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

  camera = std::make_shared<Camera>(vulkanEngineOptions.CAMERA_POSITION);
  camera->setSpeed(vulkanEngineOptions.CAMERA_SPEED);
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

  if (sceneIsFocused() && useCamera)
  {
    camera->processInput(window);
    setCameraParameters(camera->getPosition(), camera->getViewMatrix());
  }

  doComputing();

  doRendering();

  createNewFrame();
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
                                                             const std::shared_ptr<Model>& model)
{
  auto renderObject = std::make_shared<RenderObject>(logicalDevice->getDevice(),
                                                     physicalDevice->getPhysicalDevice(),
                                                     objectsPipeline->getLayout(), texture, specularMap, model);

  renderObjects.push_back(renderObject);

  return renderObject;
}

std::shared_ptr<Light> VulkanEngine::createLight(const glm::vec3 position, const glm::vec3 color, const float ambient,
                               const float diffuse, const float specular)
{
  auto light = std::make_shared<Light>(position, color, ambient, diffuse, specular);

  lights.push_back(light);

  return light;
}

ImGuiContext* VulkanEngine::getImGuiContext()
{
  return ImGui::GetCurrentContext();
}

bool VulkanEngine::keyIsPressed(const int key) const
{
  return window->keyIsPressed(key);
}

bool VulkanEngine::sceneIsFocused() const
{
  return isSceneFocused || !vulkanEngineOptions.USE_DOCKSPACE;
}

void VulkanEngine::renderObject(const std::shared_ptr<RenderObject>& renderObject, const PipelineType pipelineType)
{
  renderObjectsToRender[pipelineType].push_back(renderObject);
}

void VulkanEngine::renderLight(const std::shared_ptr<Light>& light)
{
  lightsToRender.push_back(light);
}

void VulkanEngine::enableCamera()
{
  useCamera = true;
}

void VulkanEngine::disableCamera()
{
  useCamera = false;
}

void VulkanEngine::setCameraParameters(const glm::vec3 position, const glm::mat4& viewMatrix)
{
  viewPosition = position;
  this->viewMatrix = viewMatrix;
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
  allocateCommandBuffers(computeCommandBuffers);
  allocateCommandBuffers(offscreenCommandBuffers);
  allocateCommandBuffers(swapchainCommandBuffers);

  swapChain = std::make_shared<SwapChain>(physicalDevice, logicalDevice, window);

  renderPass = std::make_shared<RenderPass>(logicalDevice->getDevice(), physicalDevice->getPhysicalDevice(),
                                            swapChain->getImageFormat(), physicalDevice->getMsaaSamples(),
                                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  offscreenRenderPass = std::make_shared<RenderPass>(logicalDevice->getDevice(),
                                                     physicalDevice->getPhysicalDevice(), VK_FORMAT_B8G8R8A8_UNORM,
                                                     physicalDevice->getMsaaSamples(),
                                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  objectsPipeline = std::make_unique<ObjectsPipeline>(physicalDevice, logicalDevice, renderPass);

  ellipticalDotsPipeline = std::make_unique<EllipticalDots>(physicalDevice, logicalDevice, renderPass);

  guiPipeline = std::make_unique<GuiPipeline>(physicalDevice, logicalDevice, renderPass,
                                              vulkanEngineOptions.MAX_IMGUI_TEXTURES);

  if (vulkanEngineOptions.DO_DOTS)
  {
    dotsPipeline = std::make_unique<DotsPipeline>(physicalDevice, logicalDevice, commandPool,
                                                  renderPass->getRenderPass(), swapChain->getExtent());
  }

  imGuiInstance = std::make_unique<ImGuiInstance>(commandPool, window, instance, physicalDevice, logicalDevice,
                                                  renderPass, guiPipeline, vulkanEngineOptions.USE_DOCKSPACE);

  framebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, swapChain, commandPool, renderPass,
                                              swapChain->getExtent());

  if (vulkanEngineOptions.USE_DOCKSPACE)
  {
    offscreenFramebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, nullptr, commandPool,
                                                         renderPass, swapChain->getExtent());
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

void VulkanEngine::allocateCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) const
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

void VulkanEngine::recordCommandBuffer(const VkCommandBuffer& commandBuffer, const uint32_t imageIndex,
                                       const std::function<void(const VkCommandBuffer& cmdBuffer, uint32_t imgIndex)>& renderFunction)
{
  constexpr VkCommandBufferBeginInfo beginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
  };

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  renderFunction(commandBuffer, imageIndex);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to record command buffer!");
  }
}

void VulkanEngine::recordComputeCommandBuffer(const VkCommandBuffer& commandBuffer) const
{
  recordCommandBuffer(commandBuffer, currentFrame, [this](const VkCommandBuffer& cmdBuffer,
                      const uint32_t imgIndex)
  {
    if (vulkanEngineOptions.DO_DOTS)
    {
      dotsPipeline->compute(cmdBuffer, imgIndex);
    }
  });
}

void VulkanEngine::recordOffscreenCommandBuffer(const VkCommandBuffer& commandBuffer, const uint32_t imageIndex) const
{
  recordCommandBuffer(commandBuffer, imageIndex, [this](const VkCommandBuffer& cmdBuffer,
                      const uint32_t imgIndex)
  {
    if (!vulkanEngineOptions.USE_DOCKSPACE ||
        offscreenViewportExtent.width == 0 || offscreenViewportExtent.height == 0)
    {
      return;
    }

    offscreenRenderPass->begin(offscreenFramebuffer->getFramebuffer(imgIndex), offscreenViewportExtent, cmdBuffer);

    renderGraphicsPipelines(cmdBuffer, offscreenViewportExtent);

    RenderPass::end(cmdBuffer);
  });
}

void VulkanEngine::recordSwapchainCommandBuffer(const VkCommandBuffer& commandBuffer, const uint32_t imageIndex) const
{
  recordCommandBuffer(commandBuffer, imageIndex, [this](const VkCommandBuffer& cmdBuffer,
                      const uint32_t imgIndex)
  {
    renderPass->begin(framebuffer->getFramebuffer(imgIndex), swapChain->getExtent(), cmdBuffer);

    if (!vulkanEngineOptions.USE_DOCKSPACE)
    {
      renderGraphicsPipelines(cmdBuffer, swapChain->getExtent());
    }

    guiPipeline->render(cmdBuffer, swapChain->getExtent());

    RenderPass::end(cmdBuffer);
  });
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

  renderGuiScene(imageIndex);

  logicalDevice->resetGraphicsFences(currentFrame);

  vkResetCommandBuffer(offscreenCommandBuffers[currentFrame], 0);
  recordOffscreenCommandBuffer(offscreenCommandBuffers[currentFrame], imageIndex);
  logicalDevice->submitOffscreenGraphicsQueue(currentFrame, &offscreenCommandBuffers[currentFrame]);

  vkResetCommandBuffer(swapchainCommandBuffers[currentFrame], 0);
  recordSwapchainCommandBuffer(swapchainCommandBuffers[currentFrame], imageIndex);
  logicalDevice->submitGraphicsQueue(currentFrame, &swapchainCommandBuffers[currentFrame]);

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
  framebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, swapChain, commandPool, renderPass,
                                              swapChain->getExtent());

  if (vulkanEngineOptions.USE_DOCKSPACE)
  {
    if (offscreenViewportExtent.width == 0 || offscreenViewportExtent.height == 0)
    {
      return;
    }

    offscreenFramebuffer.reset();

    offscreenFramebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, nullptr, commandPool,
                                                         renderPass, offscreenViewportExtent);
  }
}

void VulkanEngine::renderGuiScene(const uint32_t imageIndex)
{
  if (!vulkanEngineOptions.USE_DOCKSPACE)
  {
    return;
  }

  ImGui::Begin(vulkanEngineOptions.SCENE_VIEW_NAME);

  isSceneFocused = ImGui::IsWindowFocused();

  const auto contentRegionAvailable = ImGui::GetContentRegionAvail();

  const VkExtent2D currentOffscreenViewportExtent {
    .width = static_cast<uint32_t>(std::max(0.0f, contentRegionAvailable.x)),
    .height = static_cast<uint32_t>(std::max(0.0f, contentRegionAvailable.y))
  };

  if (currentOffscreenViewportExtent.width == 0 || currentOffscreenViewportExtent.height == 0)
  {
    offscreenViewportExtent = currentOffscreenViewportExtent;
    ImGui::End();
    return;
  }

  if (offscreenViewportExtent.width != currentOffscreenViewportExtent.width ||
      offscreenViewportExtent.height != currentOffscreenViewportExtent.height)
  {
    offscreenViewportExtent = currentOffscreenViewportExtent;

    logicalDevice->waitIdle();
    offscreenFramebuffer.reset();
    offscreenFramebuffer = std::make_shared<Framebuffer>(physicalDevice, logicalDevice, nullptr, commandPool,
                                                         renderPass, offscreenViewportExtent);
  }

  ImGui::Image(reinterpret_cast<ImTextureID>(offscreenFramebuffer->getFramebufferImageDescriptorSet(imageIndex)),
              contentRegionAvailable);

  ImGui::End();
}

void VulkanEngine::renderGraphicsPipelines(const VkCommandBuffer& commandBuffer, const VkExtent2D extent) const
{
  if (renderObjectsToRender.contains(PipelineType::object))
  {
    objectsPipeline->render(commandBuffer, currentFrame, viewPosition, viewMatrix, extent, lightsToRender,
                          renderObjectsToRender.at(PipelineType::object));
  }

  if (renderObjectsToRender.contains(PipelineType::ellipticalDots))
  {
    ellipticalDotsPipeline->render(commandBuffer, currentFrame, viewPosition, viewMatrix, extent, lightsToRender,
                                   renderObjectsToRender.at(PipelineType::ellipticalDots));
  }

  if (vulkanEngineOptions.DO_DOTS)
  {
    dotsPipeline->render(commandBuffer, currentFrame, extent);
  }
}


void VulkanEngine::createNewFrame()
{
  imGuiInstance->createNewFrame();

  renderObjectsToRender.clear();
  lightsToRender.clear();
}
