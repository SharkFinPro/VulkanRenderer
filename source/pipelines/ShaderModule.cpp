#include "ShaderModule.h"
#include "../components/LogicalDevice.h"
#include <fstream>

ShaderModule::ShaderModule(const std::shared_ptr<LogicalDevice>& logicalDevice, const char* filename,
                           const VkShaderStageFlagBits stage)
  : logicalDevice(logicalDevice), stage(stage)
{
  createShaderModule(filename);
}

ShaderModule::~ShaderModule()
{
  logicalDevice->destroyShaderModule(module);
}

VkPipelineShaderStageCreateInfo ShaderModule::getShaderStageCreateInfo() const
{
  const VkPipelineShaderStageCreateInfo shaderStageCreateInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = stage,
    .module = module,
    .pName = "main"
  };

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
  file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

  file.close();

  return buffer;
}

void ShaderModule::createShaderModule(const char* file)
{
  const auto code = readFile(file);

  const VkShaderModuleCreateInfo shaderModuleCreateInfo {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = code.size(),
    .pCode = reinterpret_cast<const uint32_t*>(code.data())
  };

  module = logicalDevice->createShaderModule(shaderModuleCreateInfo);
}
