#ifndef MOUSEPICKER_H
#define MOUSEPICKER_H

#include "../pipelines/custom/config/PipelineTypes.h"
#include "../pipelines/custom/MousePickingPipeline.h"
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

class LogicalDevice;
class CommandBuffer;
class StandardFramebuffer;
class RenderObject;
class RenderPass;
class Window;

class MousePicker {
public:
  MousePicker(const std::shared_ptr<LogicalDevice>& logicalDevice,
              const std::shared_ptr<Window>& window,
              const VkCommandPool& commandPool,
              VkDescriptorSetLayout objectDescriptorSetLayout);

  void clearObjectsToMousePick();

  void recreateFramebuffer(VkExtent2D viewportExtent);

  void doMousePicking(uint32_t imageIndex,
                      uint32_t currentFrame,
                      glm::vec3 viewPosition,
                      const glm::mat4& viewMatrix,
                      std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& renderObjectsToRender);

  [[nodiscard]] bool canMousePick() const;

  void renderObject(const std::shared_ptr<RenderObject>& renderObject, bool* mousePicked);

  void setViewportPos(ImVec2 viewportPos);

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;
  std::shared_ptr<Window> m_window;

  VkExtent2D m_viewportExtent { 1, 1 };

  ImVec2 m_viewportPos {0, 0};

  std::shared_ptr<CommandBuffer> m_mousePickingCommandBuffer;

  std::shared_ptr<RenderPass> m_mousePickingRenderPass;
  std::unique_ptr<MousePickingPipeline> m_mousePickingPipeline;
  std::shared_ptr<StandardFramebuffer> m_mousePickingFramebuffer;

  std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>> m_renderObjectsToMousePick;
  std::unordered_map<uint32_t, bool*> m_mousePickingItems;

  bool m_canMousePick = false;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  void recordMousePickingCommandBuffer(uint32_t imageIndex,
                                       uint32_t currentFrame,
                                       glm::vec3 viewPosition,
                                       const glm::mat4& viewMatrix) const;

  bool validateMousePickingMousePosition(int32_t& mouseX, int32_t& mouseY);

  [[nodiscard]] uint32_t getIDFromMousePickingFramebuffer(int32_t mouseX, int32_t mouseY) const;

  [[nodiscard]] uint32_t getObjectIDFromBuffer(VkDeviceMemory stagingBufferMemory) const;

  void handleMousePickingResult(uint32_t objectID,
                                std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& renderObjectsToRender);
};



#endif //MOUSEPICKER_H
