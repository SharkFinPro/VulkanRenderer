#ifndef VULKANPROJECT_SHADERMODULE_H
#define VULKANPROJECT_SHADERMODULE_H

#include <vulkan/vulkan.h>
#include <vector>

class ShaderModule {
public:
  ShaderModule(VkDevice& device, const char* filename, VkShaderStageFlagBits stage);
  ~ShaderModule();

  [[nodiscard]] VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

private:
  VkDevice& device;
  VkShaderStageFlagBits stage;
  VkShaderModule module;

  static std::vector<char> readFile(const char* filename);

  void createShaderModule(const char* file);
};


#endif //VULKANPROJECT_SHADERMODULE_H
