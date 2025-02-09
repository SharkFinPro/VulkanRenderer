#ifndef VULKANPROJECT_SHADERMODULE_H
#define VULKANPROJECT_SHADERMODULE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class LogicalDevice;

class ShaderModule {
public:
  ShaderModule(const std::shared_ptr<LogicalDevice>& logicalDevice, const char* filename, VkShaderStageFlagBits stage);
  ~ShaderModule();

  [[nodiscard]] VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

private:
  std::shared_ptr<LogicalDevice> logicalDevice;

  VkShaderStageFlagBits stage{};
  VkShaderModule module = VK_NULL_HANDLE;

  static std::vector<char> readFile(const char* filename);

  void createShaderModule(const char* file);
};


#endif //VULKANPROJECT_SHADERMODULE_H
