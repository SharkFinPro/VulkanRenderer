#ifndef VULKANPROJECT_SHADERMODULE_H
#define VULKANPROJECT_SHADERMODULE_H

#include <vulkan/vulkan.h>
#include <vector>

class ShaderModule {
public:
  ShaderModule(VkDevice& device, const char* filename, VkShaderStageFlagBits stage);
  ~ShaderModule();

  VkPipelineShaderStageCreateInfo getShaderStageCreateInfo();

private:
  std::vector<char> readFile(const char* filename);

  void createShaderModule(const char* file);

private:
  VkDevice& device;
  VkShaderStageFlagBits stage;
  VkShaderModule module;
};


#endif //VULKANPROJECT_SHADERMODULE_H
