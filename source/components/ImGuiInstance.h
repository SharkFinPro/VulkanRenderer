#ifndef VULKANPROJECT_IMGUIINSTANCE_H
#define VULKANPROJECT_IMGUIINSTANCE_H

#include <memory>
#include <vulkan/vulkan.h>

class Window;
class Instance;
class PhysicalDevice;
class LogicalDevice;
class RenderPass;
class GuiPipeline;

class ImGuiInstance {
public:
  ImGuiInstance(VkCommandPool& commandPool, const std::shared_ptr<Window>& window, const std::unique_ptr<Instance>& instance,
                const std::shared_ptr<PhysicalDevice>& physicalDevice, const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass, const std::unique_ptr<GuiPipeline>& guiPipeline);
  ~ImGuiInstance();
};


#endif //VULKANPROJECT_IMGUIINSTANCE_H
