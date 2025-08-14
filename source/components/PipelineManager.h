#ifndef PIPELINEMANAGER_H
#define PIPELINEMANAGER_H

#include "../pipelines/custom/BendyPipeline.h"
#include "../pipelines/custom/LinePipeline.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vke {

class MousePicker;
class LightingManager;
class DotsPipeline;
class GuiPipeline;
class Pipeline;
enum class PipelineType;
class RenderObject;
class SmokePipeline;

class PipelineManager {
public:
  PipelineManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass,
                  const std::shared_ptr<LightingManager>& lightingManager,
                   const std::shared_ptr<MousePicker>& mousePicker,
                  VkDescriptorSetLayout objectDescriptorSetLayout,
                  VkDescriptorPool descriptorPool,
                  VkCommandPool commandPool,
                  bool shouldDoDots);

  void createNewFrame();

  void renderObject(const std::shared_ptr<RenderObject>& renderObject, PipelineType pipelineType, bool* mousePicked = nullptr);
  void renderLine(glm::vec3 start, glm::vec3 end);

  void renderBendyPlant(const BendyPlant& bendyPlant) const;

  std::shared_ptr<SmokePipeline> createSmokeSystem(glm::vec3 position = glm::vec3(0.0f),
                                                   uint32_t numParticles = 5'000'000);

  void destroySmokeSystem(const std::shared_ptr<SmokePipeline>& smokeSystem);

  [[nodiscard]] std::shared_ptr<DotsPipeline> getDotsPipeline();

  [[nodiscard]] std::vector<std::shared_ptr<SmokePipeline>>& getSmokeSystems();

  [[nodiscard]] std::shared_ptr<GuiPipeline> getGuiPipeline();

  void renderGraphicsPipelines(const std::shared_ptr<CommandBuffer>& commandBuffer,
                               VkExtent2D extent,
                               uint32_t currentFrame,
                               const glm::vec3& viewPosition,
                               const glm::mat4& viewMatrix) const;

  [[nodiscard]] std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& getRenderObjectsToRender();

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  std::shared_ptr<RenderPass> m_renderPass;

  std::shared_ptr<LightingManager> m_lightingManager;

  std::shared_ptr<MousePicker> m_mousePicker;

  std::shared_ptr<GuiPipeline> m_guiPipeline;
  std::shared_ptr<DotsPipeline> m_dotsPipeline;

  std::unordered_map<PipelineType, std::unique_ptr<Pipeline>> m_pipelines;
  std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> m_renderObjectsToRender;

  std::vector<std::shared_ptr<SmokePipeline>> m_smokeSystems;

  std::unique_ptr<LinePipeline> m_linePipeline;
  std::vector<LineVertex> m_lineVerticesToRender;

  std::unique_ptr<BendyPipeline> m_bendyPipeline;

  bool m_shouldDoDots;

  void createPipelines(VkDescriptorSetLayout objectDescriptorSetLayout);

  void renderRenderObjects(const RenderInfo& renderInfo) const;

  void renderSmokeSystems(const RenderInfo& renderInfo) const;
};

} // namespace vke

#endif //PIPELINEMANAGER_H
