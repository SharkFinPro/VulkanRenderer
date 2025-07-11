#include "VulkanEngine.h"
#include "utilities/Buffers.h"
#include "utilities/Images.h"
#include "pipelines/custom/BumpyCurtain.h"
#include "pipelines/custom/CubeMapPipeline.h"
#include "pipelines/custom/CurtainPipeline.h"
#include "pipelines/custom/ObjectsPipeline.h"
#include "pipelines/custom/EllipticalDots.h"
#include "pipelines/custom/NoisyEllipticalDots.h"
#include "pipelines/custom/TexturedPlane.h"
#include "pipelines/custom/MagnifyWhirlMosaicPipeline.h"
#include "pipelines/custom/ObjectHighlightPipeline.h"
#include "pipelines/custom/SnakePipeline.h"
#include "pipelines/custom/CrossesPipeline.h"
#include <ranges>
#include <stdexcept>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

VulkanEngine::VulkanEngine(VulkanEngineOptions vulkanEngineOptions)
  : vulkanEngineOptions(vulkanEngineOptions), currentFrame(0), framebufferResized(false), isSceneFocused(false),
    useCamera(true)
{
  glfwInit();
  initVulkan();

  camera = std::make_shared<Camera>(vulkanEngineOptions.CAMERA_POSITION);
  camera->setSpeed(vulkanEngineOptions.CAMERA_SPEED);
}

VulkanEngine::~VulkanEngine()
{
  logicalDevice->waitIdle();

  logicalDevice->destroyDescriptorPool(descriptorPool);

  logicalDevice->destroyDescriptorSetLayout(objectDescriptorSetLayout);

  logicalDevice->destroyCommandPool(commandPool);

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

std::shared_ptr<Texture2D> VulkanEngine::loadTexture(const char* path, const bool repeat)
{
  auto texture = std::make_shared<Texture2D>(logicalDevice, commandPool, path,
                                             repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
  textures.push_back(texture);

  return texture;
}

std::shared_ptr<Model> VulkanEngine::loadModel(const char* path, glm::vec3 rotation)
{
  auto model = std::make_shared<Model>(
    logicalDevice,
    commandPool,
    path,
    rotation
  );

  models.push_back(model);

  return model;
}

std::shared_ptr<RenderObject> VulkanEngine::loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                             const std::shared_ptr<Texture2D>& specularMap,
                                                             const std::shared_ptr<Model>& model)
{
  auto renderObject = std::make_shared<RenderObject>(
    logicalDevice,
    objectDescriptorSetLayout,
    texture,
    specularMap,
    model
  );

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

bool VulkanEngine::buttonIsPressed(const int button) const
{
  return window->buttonDown(button);
}

bool VulkanEngine::sceneIsFocused() const
{
  return isSceneFocused || !vulkanEngineOptions.USE_DOCKSPACE;
}

void VulkanEngine::renderObject(const std::shared_ptr<RenderObject>& renderObject, const PipelineType pipelineType,
                                bool* mousePicked)
{
  renderObjectsToRender[pipelineType].push_back(renderObject);

  if (mousePicked == nullptr)
  {
    return;
  }

  uint32_t objectID = static_cast<uint32_t>(renderObjectsToMousePick.size()) + 1;
  renderObjectsToMousePick.emplace_back( renderObject, objectID );
  mousePickingItems[objectID] = mousePicked;
  *mousePicked = false;
}

void VulkanEngine::renderLight(const std::shared_ptr<Light>& light)
{
  lightsToRender.push_back(light);
}

void VulkanEngine::renderLine(const glm::vec3 start, const glm::vec3 end)
{
  lineVerticesToRender.push_back({start});
  lineVerticesToRender.push_back({end});
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

std::shared_ptr<ImGuiInstance> VulkanEngine::getImGuiInstance() const
{
  return imGuiInstance;
}

std::shared_ptr<SmokePipeline> VulkanEngine::createSmokeSystem(const glm::vec3 position, const uint32_t numParticles)
{
  auto system = std::make_shared<SmokePipeline>(physicalDevice, logicalDevice, commandPool, renderPass->getRenderPass(),
                                                descriptorPool, position, numParticles);

  smokeSystems.push_back(system);

  return system;
}

void VulkanEngine::destroySmokeSystem(const std::shared_ptr<SmokePipeline>& smokeSystem)
{
  const auto system = std::ranges::find(smokeSystems, smokeSystem);

  if (system == smokeSystems.end())
  {
    return;
  }

  logicalDevice->waitIdle();
  smokeSystems.erase(system);
}

bool VulkanEngine::canMousePick() const
{
  return m_canMousePick;
}

void VulkanEngine::initVulkan()
{
  instance = std::make_shared<Instance>();

  window = std::make_shared<Window>(vulkanEngineOptions.WINDOW_WIDTH, vulkanEngineOptions.WINDOW_HEIGHT,
                                    vulkanEngineOptions.WINDOW_TITLE, instance,
                                    vulkanEngineOptions.FULLSCREEN);

  physicalDevice = std::make_shared<PhysicalDevice>(instance, window->getSurface());

  logicalDevice = std::make_shared<LogicalDevice>(physicalDevice);

  createCommandPool();

  computeCommandBuffer = std::make_shared<CommandBuffer>(logicalDevice, commandPool);
  offscreenCommandBuffer = std::make_shared<CommandBuffer>(logicalDevice, commandPool);
  swapchainCommandBuffer = std::make_shared<CommandBuffer>(logicalDevice, commandPool);
  mousePickingCommandBuffer = std::make_shared<CommandBuffer>(logicalDevice, commandPool);

  createDescriptorPool();

  createObjectDescriptorSetLayout();

  swapChain = std::make_shared<SwapChain>(logicalDevice, window);

  renderPass = std::make_shared<RenderPass>(logicalDevice, physicalDevice, swapChain->getImageFormat(),
                                            physicalDevice->getMsaaSamples(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  offscreenRenderPass = std::make_shared<RenderPass>(logicalDevice, physicalDevice, VK_FORMAT_B8G8R8A8_UNORM,
                                                     physicalDevice->getMsaaSamples(),
                                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  mousePickingRenderPass = std::make_shared<RenderPass>(logicalDevice, physicalDevice, VK_FORMAT_R8G8B8A8_UNORM,
                                                        VK_SAMPLE_COUNT_1_BIT,
                                                        VK_IMAGE_LAYOUT_UNDEFINED);

  pipelines[PipelineType::object] = std::make_unique<ObjectsPipeline>(
    physicalDevice, logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::objectHighlight] = std::make_unique<ObjectHighlightPipeline>(
    physicalDevice, logicalDevice, renderPass, objectDescriptorSetLayout);

  pipelines[PipelineType::ellipticalDots] = std::make_unique<EllipticalDots>(
    physicalDevice, logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::noisyEllipticalDots] = std::make_unique<NoisyEllipticalDots>(
    physicalDevice, logicalDevice, renderPass, commandPool, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::bumpyCurtain] = std::make_unique<BumpyCurtain>(
    physicalDevice, logicalDevice, renderPass, commandPool, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::curtain] = std::make_unique<CurtainPipeline>(
    physicalDevice, logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::cubeMap] = std::make_unique<CubeMapPipeline>(
    physicalDevice, logicalDevice, renderPass, commandPool, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::texturedPlane] = std::make_unique<TexturedPlane>(
    physicalDevice, logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::magnifyWhirlMosaic] = std::make_unique<MagnifyWhirlMosaicPipeline>(
    physicalDevice, logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::snake] = std::make_unique<SnakePipeline>(
    physicalDevice, logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout);

  pipelines[PipelineType::crosses] = std::make_unique<CrossesPipeline>(
    physicalDevice, logicalDevice, renderPass, descriptorPool, objectDescriptorSetLayout);

  guiPipeline = std::make_unique<GuiPipeline>(physicalDevice, logicalDevice, renderPass,
                                              vulkanEngineOptions.MAX_IMGUI_TEXTURES);

  mousePickingPipeline = std::make_unique<MousePickingPipeline>(physicalDevice, logicalDevice, mousePickingRenderPass,
                                                                objectDescriptorSetLayout);

  if (vulkanEngineOptions.DO_DOTS)
  {
    dotsPipeline = std::make_unique<DotsPipeline>(physicalDevice, logicalDevice, commandPool,
                                                  renderPass->getRenderPass(), swapChain->getExtent(), descriptorPool);
  }

  linePipeline = std::make_unique<LinePipeline>(physicalDevice, logicalDevice, renderPass, descriptorPool);

  imGuiInstance = std::make_shared<ImGuiInstance>(window, instance, logicalDevice, renderPass, guiPipeline,
                                                  vulkanEngineOptions.USE_DOCKSPACE);

  framebuffer = std::make_shared<SwapchainFramebuffer>(logicalDevice, swapChain, commandPool, renderPass,
                                                       swapChain->getExtent());

  if (vulkanEngineOptions.USE_DOCKSPACE)
  {
    offscreenFramebuffer = std::make_shared<StandardFramebuffer>(logicalDevice, commandPool, renderPass,
                                                                 swapChain->getExtent());
  }

  mousePickingFramebuffer = std::make_shared<StandardFramebuffer>(logicalDevice, commandPool, mousePickingRenderPass,
                                                                  swapChain->getExtent(), true);
}

void VulkanEngine::createCommandPool()
{
  const auto queueFamilyIndices = physicalDevice->getQueueFamilies();

  const VkCommandPoolCreateInfo poolInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
  };

  commandPool = logicalDevice->createCommandPool(poolInfo);
}

void VulkanEngine::recordComputeCommandBuffer() const
{
  computeCommandBuffer->record([this]()
  {
    if (vulkanEngineOptions.DO_DOTS)
    {
      dotsPipeline->compute(computeCommandBuffer, currentFrame);
    }

    for (const auto& system : smokeSystems)
    {
      system->compute(computeCommandBuffer, currentFrame);
    }
  });
}

void VulkanEngine::recordMousePickingCommandBuffer(const uint32_t imageIndex) const
{
  mousePickingCommandBuffer->record([this, imageIndex]()
  {
    const RenderInfo renderInfo {
      .commandBuffer = mousePickingCommandBuffer,
      .currentFrame = currentFrame,
      .viewPosition = viewPosition,
      .viewMatrix = viewMatrix,
      .extent = offscreenViewportExtent,
      .lights = lights
    };

    mousePickingRenderPass->begin(mousePickingFramebuffer->getFramebuffer(imageIndex), offscreenViewportExtent, renderInfo.commandBuffer);

    const std::vector<std::shared_ptr<Light>> lights{};

    const VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(renderInfo.extent.width),
      .height = static_cast<float>(renderInfo.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };
    renderInfo.commandBuffer->setViewport(viewport);

    const VkRect2D scissor = {
      .offset = {0, 0},
      .extent = renderInfo.extent
    };
    renderInfo.commandBuffer->setScissor(scissor);

    mousePickingPipeline->render(&renderInfo, &renderObjectsToMousePick);

    mousePickingCommandBuffer->endRenderPass();
  });
}

void VulkanEngine::recordOffscreenCommandBuffer(const uint32_t imageIndex) const
{
  offscreenCommandBuffer->record([this, imageIndex]()
  {
    if (!vulkanEngineOptions.USE_DOCKSPACE ||
        offscreenViewportExtent.width == 0 || offscreenViewportExtent.height == 0)
    {
      return;
    }

    offscreenRenderPass->begin(offscreenFramebuffer->getFramebuffer(imageIndex), offscreenViewportExtent, offscreenCommandBuffer);

    renderGraphicsPipelines(offscreenCommandBuffer, offscreenViewportExtent);

    offscreenCommandBuffer->endRenderPass();
  });
}

void VulkanEngine::recordSwapchainCommandBuffer(const uint32_t imageIndex) const
{
  swapchainCommandBuffer->record([this, imageIndex]()
  {
    const RenderInfo renderInfo {
      .commandBuffer = swapchainCommandBuffer,
      .currentFrame = currentFrame,
      .viewPosition = viewPosition,
      .viewMatrix = viewMatrix,
      .extent = swapChain->getExtent(),
      .lights = lightsToRender
    };

    renderPass->begin(framebuffer->getFramebuffer(imageIndex), swapChain->getExtent(), renderInfo.commandBuffer);

    if (!vulkanEngineOptions.USE_DOCKSPACE)
    {
      renderGraphicsPipelines(renderInfo.commandBuffer, swapChain->getExtent());
    }

    const VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(renderInfo.extent.width),
      .height = static_cast<float>(renderInfo.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };
    renderInfo.commandBuffer->setViewport(viewport);

    const VkRect2D scissor = {
      .offset = {0, 0},
      .extent = renderInfo.extent
    };
    renderInfo.commandBuffer->setScissor(scissor);

    guiPipeline->render(&renderInfo);

    renderInfo.commandBuffer->endRenderPass();
  });
}

void VulkanEngine::doComputing() const
{
  logicalDevice->waitForComputeFences(currentFrame);

  logicalDevice->resetComputeFences(currentFrame);

  computeCommandBuffer->setCurrentFrame(currentFrame);
  computeCommandBuffer->resetCommandBuffer();
  recordComputeCommandBuffer();

  logicalDevice->submitComputeQueue(currentFrame, computeCommandBuffer->getCommandBuffer());
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

  mousePickingCommandBuffer->setCurrentFrame(currentFrame);
  mousePickingCommandBuffer->resetCommandBuffer();
  recordMousePickingCommandBuffer(imageIndex);
  logicalDevice->submitMousePickingGraphicsQueue(currentFrame, mousePickingCommandBuffer->getCommandBuffer());
  logicalDevice->waitForMousePickingFences(currentFrame);
  doMousePicking();

  offscreenCommandBuffer->setCurrentFrame(currentFrame);
  offscreenCommandBuffer->resetCommandBuffer();
  recordOffscreenCommandBuffer(imageIndex);
  logicalDevice->submitOffscreenGraphicsQueue(currentFrame, offscreenCommandBuffer->getCommandBuffer());

  swapchainCommandBuffer->setCurrentFrame(currentFrame);
  swapchainCommandBuffer->resetCommandBuffer();
  recordSwapchainCommandBuffer(imageIndex);
  logicalDevice->submitGraphicsQueue(currentFrame, swapchainCommandBuffer->getCommandBuffer());

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

  currentFrame = (currentFrame + 1) % logicalDevice->getMaxFramesInFlight();
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

  mousePickingFramebuffer.reset();
  framebuffer.reset();
  swapChain.reset();

  physicalDevice->updateSwapChainSupportDetails();

  swapChain = std::make_shared<SwapChain>(logicalDevice, window);
  framebuffer = std::make_shared<SwapchainFramebuffer>(logicalDevice, swapChain, commandPool, renderPass,
                                                       swapChain->getExtent());

  mousePickingFramebuffer = std::make_shared<StandardFramebuffer>(logicalDevice, commandPool, mousePickingRenderPass,
                                                                  offscreenViewportExtent, true);

  if (vulkanEngineOptions.USE_DOCKSPACE)
  {
    if (offscreenViewportExtent.width == 0 || offscreenViewportExtent.height == 0)
    {
      return;
    }

    offscreenFramebuffer.reset();

    offscreenFramebuffer = std::make_shared<StandardFramebuffer>(logicalDevice, commandPool, renderPass,
                                                                 offscreenViewportExtent);
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
    offscreenFramebuffer = std::make_shared<StandardFramebuffer>(logicalDevice, commandPool, renderPass,
                                                                 offscreenViewportExtent);

    mousePickingFramebuffer.reset();
    mousePickingFramebuffer = std::make_shared<StandardFramebuffer>(logicalDevice, commandPool, mousePickingRenderPass,
                                                                    offscreenViewportExtent, true);
  }

  offscreenViewportPos = ImGui::GetCursorScreenPos();

  ImGui::Image(reinterpret_cast<ImTextureID>(offscreenFramebuffer->getFramebufferImageDescriptorSet(imageIndex)),
              contentRegionAvailable);

  ImGui::End();
}

void VulkanEngine::renderGraphicsPipelines(const std::shared_ptr<CommandBuffer>& commandBuffer, const VkExtent2D extent) const
{
  const RenderInfo renderInfo {
    .commandBuffer = commandBuffer,
    .currentFrame = currentFrame,
    .viewPosition = viewPosition,
    .viewMatrix = viewMatrix,
    .extent = extent,
    .lights = lightsToRender
  };

  const VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(extent.width),
    .height = static_cast<float>(extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  renderInfo.commandBuffer->setViewport(viewport);

  const VkRect2D scissor = {
    .offset = {0, 0},
    .extent = extent
  };
  renderInfo.commandBuffer->setScissor(scissor);

  renderRenderObjects(renderInfo);

  if (vulkanEngineOptions.DO_DOTS)
  {
    dotsPipeline->render(&renderInfo, nullptr);
  }

  linePipeline->render(&renderInfo, commandPool, lineVerticesToRender);

  renderSmokeSystems(renderInfo);
}

void VulkanEngine::renderRenderObjects(const RenderInfo& renderInfo) const
{
  for (const auto& [type, objects] : renderObjectsToRender)
  {
    if (objects.empty())
    {
      continue;
    }

    if (auto it = pipelines.find(type); it != pipelines.end())
    {
      if (auto* graphicsPipeline = dynamic_cast<GraphicsPipeline*>(it->second.get()))
      {
        graphicsPipeline->displayGui();
        graphicsPipeline->render(&renderInfo, &objects);
        continue;
      }

      throw std::runtime_error("Pipeline for object type is not a GraphicsPipeline");
    }

    throw std::runtime_error("Pipeline for object type does not exist");
  }
}

void VulkanEngine::renderSmokeSystems(const RenderInfo& renderInfo) const
{
  if (!smokeSystems.empty())
  {
    ImGui::Begin("Smoke");
    ImGui::Separator();
    for (const auto& system : smokeSystems)
    {
      ImGui::PushID(&system);
      system->displayGui();
      ImGui::PopID();

      ImGui::Separator();

      system->render(&renderInfo, nullptr);
    }
    ImGui::End();
  }
}

void VulkanEngine::createNewFrame()
{
  imGuiInstance->createNewFrame();

  renderObjectsToRender.clear();
  lightsToRender.clear();
  lineVerticesToRender.clear();

  renderObjectsToMousePick.clear();
}

void VulkanEngine::createDescriptorPool()
{
  const std::array<VkDescriptorPoolSize, 3> poolSizes {{
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, logicalDevice->getMaxFramesInFlight() * 30},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, logicalDevice->getMaxFramesInFlight() * 50},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, logicalDevice->getMaxFramesInFlight() * 10}
  }};

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = logicalDevice->getMaxFramesInFlight() * 20,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  descriptorPool = logicalDevice->createDescriptorPool(poolCreateInfo);
}

void VulkanEngine::createObjectDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding transformLayout {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding textureLayout {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding specularLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array objectBindings {
    transformLayout,
    textureLayout,
    specularLayout
  };

  const VkDescriptorSetLayoutCreateInfo objectLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(objectBindings.size()),
    .pBindings = objectBindings.data()
  };

  objectDescriptorSetLayout = logicalDevice->createDescriptorSetLayout(objectLayoutCreateInfo);
}

bool VulkanEngine::validateMousePickingMousePosition(int32_t& mouseX, int32_t& mouseY)
{
  if (!vulkanEngineOptions.USE_DOCKSPACE || offscreenViewportExtent.width == 0 || offscreenViewportExtent.height == 0)
  {
    m_canMousePick = false;
  }
  else
  {
    double mouseXPos, mouseYPos;
    window->getCursorPos(mouseXPos, mouseYPos);
    mouseX = static_cast<int32_t>(mouseXPos);
    mouseY = static_cast<int32_t>(mouseYPos);

    mouseX -= static_cast<int32_t>(offscreenViewportPos.x);
    mouseY -= static_cast<int32_t>(offscreenViewportPos.y);

    m_canMousePick = !(mouseX < 0 || mouseX > offscreenViewportExtent.width - 1 ||
                       mouseY < 0 || mouseY > offscreenViewportExtent.height - 1);
  }

  return m_canMousePick;
}

uint32_t VulkanEngine::getIDFromMousePickingFramebuffer(const int32_t mouseX, const int32_t mouseY) const
{
  constexpr VkDeviceSize bufferSize = 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  Buffers::createBuffer(logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        stagingBuffer, stagingBufferMemory);

  VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(logicalDevice, commandPool);

  const VkImageMemoryBarrier barrier {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = mousePickingFramebuffer->getColorImage(),
    .subresourceRange {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  Images::copyImageToBuffer(mousePickingFramebuffer->getColorImage(), { mouseX, mouseY, 0 },
                            { 1, 1, 1 }, commandBuffer, stagingBuffer);

  Buffers::endSingleTimeCommands(logicalDevice, commandPool, logicalDevice->getGraphicsQueue(), commandBuffer);

  const uint32_t objectID = getObjectIDFromBuffer(stagingBufferMemory);

  Buffers::destroyBuffer(logicalDevice, stagingBuffer, stagingBufferMemory);

  return objectID;
}

uint32_t VulkanEngine::getObjectIDFromBuffer(VkDeviceMemory stagingBufferMemory) const
{
  uint32_t objectID = 0;

  logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [&objectID](void* data) {
    const uint8_t* pixel = static_cast<uint8_t*>(data);

    objectID = pixel[0] << 16 | pixel[1] << 8 | pixel[2];
  });

  return objectID;
}

void VulkanEngine::handleMousePickingResult(const uint32_t objectID)
{
  *mousePickingItems.at(objectID) = true;

  for (auto& [object, id] : renderObjectsToMousePick)
  {
    if (id == objectID)
    {
      renderObjectsToRender[PipelineType::objectHighlight].push_back(object);
      break;
    }
  }
}

void VulkanEngine::doMousePicking()
{
  int32_t mouseX, mouseY;
  if (!validateMousePickingMousePosition(mouseX, mouseY))
  {
    return;
  }

  const auto objectID = getIDFromMousePickingFramebuffer(mouseX, mouseY);

  if (objectID == 0)
  {
    return;
  }

  handleMousePickingResult(objectID);
}
