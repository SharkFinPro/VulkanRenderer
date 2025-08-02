#ifndef VULKANPROJECT_VULKANENGINE_H
#define VULKANPROJECT_VULKANENGINE_H

#include "components/window/Window.h"
#include "pipelines/custom/config/PipelineTypes.h"
#include "pipelines/custom/BendyPipeline.h"
#include "VulkanEngineOptions.h"
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

class Camera;
class CommandBuffer;
class DotsPipeline;
class GuiPipeline;
class ImGuiInstance;
class Light;
class LightingManager;
class LinePipeline;
struct LineVertex;
class LogicalDevice;
class Model;
class MousePicker;
class PhysicalDevice;
class Pipeline;
class RenderObject;
struct RenderInfo;
class RenderPass;
class SmokePipeline;
class StandardFramebuffer;
class SwapChain;
class SwapchainFramebuffer;
class Texture;
class Texture2D;

class VulkanEngine {
public:
  explicit VulkanEngine(const VulkanEngineOptions& vulkanEngineOptions);
  ~VulkanEngine();

  [[nodiscard]] bool isActive() const;

  void render();

  std::shared_ptr<Texture2D> loadTexture(const char* path, bool repeat = true);
  std::shared_ptr<Model> loadModel(const char* path, glm::vec3 rotation = { 0, 0, 0 });
  [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                               const std::shared_ptr<Texture2D>& specularMap,
                                                               const std::shared_ptr<Model>&);

  std::shared_ptr<Light> createLight(glm::vec3 position, glm::vec3 color, float ambient, float diffuse, float specular = 1.0f) const;

  static ImGuiContext* getImGuiContext();

  [[nodiscard]] bool keyIsPressed(int key) const;

  [[nodiscard]] bool buttonIsPressed(int button) const;

  [[nodiscard]] bool sceneIsFocused() const;

  void renderObject(const std::shared_ptr<RenderObject>& renderObject, PipelineType pipelineType, bool* mousePicked = nullptr);
  void renderLight(const std::shared_ptr<Light>& light) const;
  void renderLine(glm::vec3 start, glm::vec3 end);

  void renderBendyPlant(const BendyPlant& bendyPlant) const;

  void enableCamera();
  void disableCamera();

  void setCameraParameters(glm::vec3 position, const glm::mat4& viewMatrix);

  [[nodiscard]] std::shared_ptr<ImGuiInstance> getImGuiInstance() const;

  std::shared_ptr<SmokePipeline> createSmokeSystem(glm::vec3 position = glm::vec3(0.0f),
                                                   uint32_t numParticles = 5'000'000);

  void destroySmokeSystem(const std::shared_ptr<SmokePipeline>& smokeSystem);

  [[nodiscard]] bool canMousePick() const;

private:
  std::shared_ptr<Instance> m_instance;
  std::shared_ptr<Window> m_window;
  std::shared_ptr<PhysicalDevice> m_physicalDevice;
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::unique_ptr<LightingManager> m_lightingManager;

  std::shared_ptr<SwapChain> m_swapChain;
  std::shared_ptr<RenderPass> m_renderPass;
  std::shared_ptr<RenderPass> m_offscreenRenderPass;

  std::unique_ptr<GuiPipeline> m_guiPipeline;
  std::unique_ptr<DotsPipeline> m_dotsPipeline;

  std::unordered_map<PipelineType, std::unique_ptr<Pipeline>> m_pipelines;
  std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> m_renderObjectsToRender;

  std::vector<std::shared_ptr<SmokePipeline>> m_smokeSystems;

  std::unique_ptr<LinePipeline> m_linePipeline;

  std::shared_ptr<ImGuiInstance> m_imGuiInstance;

  std::shared_ptr<SwapchainFramebuffer> m_framebuffer;
  std::shared_ptr<StandardFramebuffer> m_offscreenFramebuffer;

  std::vector<std::shared_ptr<Texture>> m_textures;
  std::vector<std::shared_ptr<Model>> m_models;
  std::vector<std::shared_ptr<RenderObject>> m_renderObjects;

  std::vector<LineVertex> m_lineVerticesToRender;

  std::shared_ptr<Camera> m_camera;

  VulkanEngineOptions m_vulkanEngineOptions;
  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  std::shared_ptr<CommandBuffer> m_offscreenCommandBuffer;
  std::shared_ptr<CommandBuffer> m_swapchainCommandBuffer;
  std::shared_ptr<CommandBuffer> m_computeCommandBuffer;

  uint32_t m_currentFrame;

  bool m_framebufferResized;

  bool m_isSceneFocused;

  VkExtent2D m_offscreenViewportExtent{};

  ImVec2 m_offscreenViewportPos{0, 0};

  bool m_useCamera;
  glm::vec3 m_viewPosition{};
  glm::mat4 m_viewMatrix{};

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<MousePicker> m_mousePicker;

  std::unique_ptr<BendyPipeline> m_bendyPipeline;

  void initVulkan();
  void createCommandPool();

  void recordComputeCommandBuffer() const;
  void recordOffscreenCommandBuffer(uint32_t imageIndex) const;
  void recordSwapchainCommandBuffer(uint32_t imageIndex) const;

  void doComputing() const;
  void doRendering();
  void recreateSwapChain();
  void renderGuiScene(uint32_t imageIndex);

  void renderGraphicsPipelines(const std::shared_ptr<CommandBuffer>& commandBuffer, VkExtent2D extent) const;

  void renderRenderObjects(const RenderInfo& renderInfo) const;

  void renderSmokeSystems(const RenderInfo& renderInfo) const;

  void createNewFrame();

  friend void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height);

  void createDescriptorPool();

  void createObjectDescriptorSetLayout();
};


#endif //VULKANPROJECT_VULKANENGINE_H
