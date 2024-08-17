#include "ShaderModule.h"
#include <fstream>

ShaderModule::ShaderModule(VkDevice& device, const char* filename, VkShaderStageFlagBits stage)
  : device(device), stage(stage), module{}
{
  createShaderModule(filename);
}

ShaderModule::~ShaderModule()
{
  vkDestroyShaderModule(device, module, nullptr);
}

VkPipelineShaderStageCreateInfo ShaderModule::getShaderStageCreateInfo()
{
  VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
  shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.stage = stage;
  shaderStageCreateInfo.module = module;
  shaderStageCreateInfo.pName = "main";

  return shaderStageCreateInfo;
}

std::vector<char> ShaderModule::readFile(const char* filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    throw std::runtime_error("failed to open file!");
  }

  const size_t fileSize = file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

void ShaderModule::createShaderModule(const char* file)
{
  const auto code = readFile(file);

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create shader module!");
  }
}
