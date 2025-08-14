#ifndef VULKANPROJECT_SHADERMODULE_H
#define VULKANPROJECT_SHADERMODULE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace vke {

class LogicalDevice;

class ShaderModule {
public:
  ShaderModule(const std::shared_ptr<LogicalDevice>& logicalDevice, const char* filename, VkShaderStageFlagBits stage);
  ~ShaderModule();

  ShaderModule(ShaderModule&& other) noexcept
    : m_logicalDevice(other.m_logicalDevice),
      m_stage(other.m_stage),
      m_module(other.m_module)
  {
    other.m_module = VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkShaderStageFlagBits m_stage{};
  VkShaderModule m_module = VK_NULL_HANDLE;

  static std::vector<char> readFile(const char* filename);

  void createShaderModule(const char* file);
};

} // namespace vke

#endif //VULKANPROJECT_SHADERMODULE_H
