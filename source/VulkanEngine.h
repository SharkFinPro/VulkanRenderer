#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <imgui.h>

#include "components/Camera.h"
#include "components/DebugMessenger.h"
#include "components/Framebuffer.h"
#include "components/ImGuiInstance.h"
#include "components/Instance.h"
#include "components/LogicalDevice.h"
#include "components/PhysicalDevice.h"
#include "components/SwapChain.h"
#include "components/Window.h"

#include "pipelines/RenderPass.h"
#include "pipelines/custom/BumpyCurtain.h"
#include "pipelines/custom/CubeMapPipeline.h"
#include "pipelines/custom/CurtainPipeline.h"
#include "pipelines/custom/ObjectsPipeline.h"
#include "pipelines/custom/GuiPipeline.h"
#include "pipelines/custom/DotsPipeline.h"
#include "pipelines/custom/EllipticalDots.h"
#include "pipelines/custom/NoisyEllipticalDots.h"
#include "pipelines/custom/TexturedPlane.h"

#include "objects/Texture.h"
#include "objects/Model.h"
#include "objects/RenderObject.h"
#include "objects/Light.h"

#include "VulkanEngineOptions.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

enum class PipelineType {
  object,
  ellipticalDots,
  noisyEllipticalDots,
  bumpyCurtain,
  curtain,
  cubeMap,
  texturedPlane
};

class VulkanEngine {
public:
  explicit VulkanEngine(VulkanEngineOptions vulkanEngineOptions);
  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  std::shared_ptr<Texture> loadTexture(const char* path);
  std::shared_ptr<Model> loadModel(const char* path, glm::vec3 rotation = { 0, 0, 0 });
  [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture>& texture,
                                                               const std::shared_ptr<Texture>& specularMap,
                                                               const std::shared_ptr<Model>&);

  std::shared_ptr<Light> createLight(glm::vec3 position, glm::vec3 color, float ambient, float diffuse, float specular = 1.0f);

  static ImGuiContext* getImGuiContext();

  [[nodiscard]] bool keyIsPressed(int key) const;

  [[nodiscard]] bool sceneIsFocused() const;

  void renderObject(const std::shared_ptr<RenderObject>& renderObject, PipelineType pipelineType);
  void renderLight(const std::shared_ptr<Light>& light);

  void enableCamera();
  void disableCamera();

  void setCameraParameters(glm::vec3 position, const glm::mat4& viewMatrix);

private:
  std::shared_ptr<Instance> instance;
  std::unique_ptr<DebugMessenger> debugMessenger;
  std::shared_ptr<Window> window;
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<SwapChain> swapChain;
  std::shared_ptr<RenderPass> renderPass;
  std::shared_ptr<RenderPass> offscreenRenderPass;
  std::unique_ptr<ObjectsPipeline> objectsPipeline;
  std::unique_ptr<GuiPipeline> guiPipeline;
  std::unique_ptr<DotsPipeline> dotsPipeline;
  std::unique_ptr<EllipticalDots> ellipticalDotsPipeline;
  std::unique_ptr<NoisyEllipticalDots> noisyEllipticalDotsPipeline;
  std::unique_ptr<CurtainPipeline> curtainPipeline;
  std::unique_ptr<BumpyCurtain> bumpyCurtainPipeline;
  std::unique_ptr<CubeMapPipeline> cubeMapPipeline;
  std::unique_ptr<TexturedPlane> texturedPlanePipeline;

  std::unique_ptr<ImGuiInstance> imGuiInstance;

  std::shared_ptr<Framebuffer> framebuffer;
  std::shared_ptr<Framebuffer> offscreenFramebuffer;

  std::vector<std::shared_ptr<Texture>> textures;
  std::vector<std::shared_ptr<Model>> models;
  std::vector<std::shared_ptr<RenderObject>> renderObjects;
  std::vector<std::shared_ptr<Light>> lights;

  std::vector<std::shared_ptr<Light>> lightsToRender;

  std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> renderObjectsToRender;


  std::shared_ptr<Camera> camera;

  VulkanEngineOptions vulkanEngineOptions;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> offscreenCommandBuffers;
  std::vector<VkCommandBuffer> swapchainCommandBuffers;
  std::vector<VkCommandBuffer> computeCommandBuffers;
  uint32_t currentFrame;

  bool framebufferResized;

  bool isSceneFocused;

  VkExtent2D offscreenViewportExtent{};

  bool useCamera;
  glm::vec3 viewPosition{};
  glm::mat4 viewMatrix{};

  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  void initVulkan();
  void createCommandPool();
  void allocateCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers) const;
  static void recordCommandBuffer(const VkCommandBuffer &commandBuffer, uint32_t imageIndex,
                                  const std::function<void(const VkCommandBuffer &cmdBuffer, uint32_t imgIndex)>& renderFunction);
  void recordComputeCommandBuffer(const VkCommandBuffer& commandBuffer) const;
  void recordOffscreenCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;
  void recordSwapchainCommandBuffer(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) const;
  void doComputing() const;
  void doRendering();
  void recreateSwapChain();
  void renderGuiScene(uint32_t imageIndex);

  void renderGraphicsPipelines(const VkCommandBuffer& commandBuffer, VkExtent2D extent) const;

  void createNewFrame();

  friend void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height);

  void createObjectDescriptorSetLayout();
};


#endif //VULKANPROJECT_VULKANENGINE_H
