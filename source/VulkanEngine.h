#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include "components/Camera.h"
#include "components/Framebuffer.h"
#include "components/ImGuiInstance.h"
#include "core/logicalDevice/LogicalDevice.h"
#include "components/SwapChain.h"
#include "components/Window.h"

#include "core/instance/DebugMessenger.h"
#include "core/instance/Instance.h"
#include "core/physicalDevice/PhysicalDevice.h"

#include "pipelines/RenderPass.h"
#include "pipelines/custom/DotsPipeline.h"
#include "pipelines/custom/GuiPipeline.h"
#include "pipelines/custom/LinePipeline.h"
#include "pipelines/custom/MousePickingPipeline.h"
#include "pipelines/custom/SmokePipeline.h"

#include "objects/Texture.h"
#include "objects/Model.h"
#include "objects/RenderObject.h"
#include "objects/Light.h"

#include "VulkanEngineOptions.h"

#include <vulkan/vulkan.h>
#include <imgui.h>

#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

enum class PipelineType {
  bumpyCurtain,
  crosses,
  curtain,
  cubeMap,
  ellipticalDots,
  magnifyWhirlMosaic,
  noisyEllipticalDots,
  object,
  objectHighlight,
  texturedPlane,
  snake
};

class VulkanEngine {
public:
  explicit VulkanEngine(VulkanEngineOptions vulkanEngineOptions);
  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  std::shared_ptr<Texture> loadTexture(const char* path, bool repeat = true);
  std::shared_ptr<Model> loadModel(const char* path, glm::vec3 rotation = { 0, 0, 0 });
  [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture>& texture,
                                                               const std::shared_ptr<Texture>& specularMap,
                                                               const std::shared_ptr<Model>&);

  std::shared_ptr<Light> createLight(glm::vec3 position, glm::vec3 color, float ambient, float diffuse, float specular = 1.0f);

  static ImGuiContext* getImGuiContext();

  [[nodiscard]] bool keyIsPressed(int key) const;

  [[nodiscard]] bool buttonIsPressed(int button) const;

  [[nodiscard]] bool sceneIsFocused() const;

  void renderObject(const std::shared_ptr<RenderObject>& renderObject, PipelineType pipelineType, bool* mousePicked = nullptr);
  void renderLight(const std::shared_ptr<Light>& light);
  void renderLine(glm::vec3 start, glm::vec3 end);

  void enableCamera();
  void disableCamera();

  void setCameraParameters(glm::vec3 position, const glm::mat4& viewMatrix);

  [[nodiscard]] std::shared_ptr<ImGuiInstance> getImGuiInstance() const;

  std::shared_ptr<SmokePipeline> createSmokeSystem(glm::vec3 position = glm::vec3(0.0f),
                                                   uint32_t numParticles = 5'000'000);

  void destroySmokeSystem(const std::shared_ptr<SmokePipeline>& smokeSystem);

  bool canMousePick() const;

private:
  std::shared_ptr<Instance> instance;
  std::unique_ptr<DebugMessenger> debugMessenger;
  std::shared_ptr<Window> window;
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<SwapChain> swapChain;
  std::shared_ptr<RenderPass> renderPass;
  std::shared_ptr<RenderPass> offscreenRenderPass;
  std::shared_ptr<RenderPass> mousePickingRenderPass;
  std::unique_ptr<GuiPipeline> guiPipeline;
  std::unique_ptr<DotsPipeline> dotsPipeline;
  std::unique_ptr<MousePickingPipeline> mousePickingPipeline;

  std::unordered_map<PipelineType, std::unique_ptr<Pipeline>> pipelines;
  std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> renderObjectsToRender;

  std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>> renderObjectsToMousePick;
  std::unordered_map<uint32_t, bool*> mousePickingItems;

  std::vector<std::shared_ptr<SmokePipeline>> smokeSystems;

  std::unique_ptr<LinePipeline> linePipeline;

  std::shared_ptr<ImGuiInstance> imGuiInstance;

  std::shared_ptr<Framebuffer> framebuffer;
  std::shared_ptr<Framebuffer> offscreenFramebuffer;
  std::shared_ptr<Framebuffer> mousePickingFramebuffer;

  std::vector<std::shared_ptr<Texture>> textures;
  std::vector<std::shared_ptr<Model>> models;
  std::vector<std::shared_ptr<RenderObject>> renderObjects;
  std::vector<std::shared_ptr<Light>> lights;

  std::vector<std::shared_ptr<Light>> lightsToRender;

  std::vector<LineVertex> lineVerticesToRender;

  std::shared_ptr<Camera> camera;

  VulkanEngineOptions vulkanEngineOptions;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> offscreenCommandBuffers;
  std::vector<VkCommandBuffer> swapchainCommandBuffers;
  std::vector<VkCommandBuffer> computeCommandBuffers;
  std::vector<VkCommandBuffer> mousePickingCommandBuffers;
  uint32_t currentFrame;

  bool framebufferResized;

  bool isSceneFocused;

  VkExtent2D offscreenViewportExtent{};

  ImVec2 offscreenViewportPos{0, 0};

  bool useCamera;
  glm::vec3 viewPosition{};
  glm::mat4 viewMatrix{};

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  bool m_canMousePick = false;

  void initVulkan();
  void createCommandPool();
  void allocateCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) const;
  static void recordCommandBuffer(const VkCommandBuffer &commandBuffer, uint32_t imageIndex,
                                  const std::function<void(const VkCommandBuffer &cmdBuffer, uint32_t imgIndex)>& renderFunction);

  void recordComputeCommandBuffer(const VkCommandBuffer& commandBuffer) const;
  void recordMousePickingCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;
  void recordOffscreenCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;
  void recordSwapchainCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;

  void doComputing() const;
  void doRendering();
  void recreateSwapChain();
  void renderGuiScene(uint32_t imageIndex);

  void renderGraphicsPipelines(const VkCommandBuffer& commandBuffer, VkExtent2D extent) const;

  void createNewFrame();

  friend void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height);

  void createDescriptorPool();

  void createObjectDescriptorSetLayout();

  bool validateMousePickingMousePosition(int32_t& mouseX, int32_t& mouseY);

  [[nodiscard]] uint32_t getIDFromMousePickingFramebuffer(int32_t mouseX, int32_t mouseY) const;

  [[nodiscard]] uint32_t getObjectIDFromBuffer(VkDeviceMemory stagingBufferMemory) const;

  void handleMousePickingResult(uint32_t objectID);

  void doMousePicking();
};


#endif //VULKANPROJECT_VULKANENGINE_H
