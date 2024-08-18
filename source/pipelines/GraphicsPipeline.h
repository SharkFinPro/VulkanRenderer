#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

#include "ShaderModule.h"
#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

class GraphicsPipeline {
public:
  GraphicsPipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice);

  virtual ~GraphicsPipeline();

protected:
  void createShader(const char* filename, VkShaderStageFlagBits stage);

  virtual void loadShaders() = 0;

  void loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
  virtual void loadDescriptorSetLayouts() {};

  void createPipelineLayout();

  void createPipeline(const VkRenderPass& renderPass);

  virtual std::unique_ptr<VkPipelineColorBlendStateCreateInfo> defineColorBlendState() { return nullptr; };

  virtual std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> defineDepthStencilState() { return nullptr; };

  virtual std::unique_ptr<VkPipelineDynamicStateCreateInfo> defineDynamicState() { return nullptr; };

  virtual std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> defineInputAssemblyState() { return nullptr; };

  virtual std::unique_ptr<VkPipelineMultisampleStateCreateInfo> defineMultisampleState() { return nullptr; };

  virtual std::unique_ptr<VkPipelineRasterizationStateCreateInfo> defineRasterizationState() { return nullptr; };

  virtual std::unique_ptr<VkPipelineTessellationStateCreateInfo> defineTessellationState() { return nullptr; };

  virtual std::unique_ptr<VkPipelineVertexInputStateCreateInfo> defineVertexInputState() { return nullptr; };

  virtual std::unique_ptr<VkPipelineViewportStateCreateInfo> defineViewportState() { return nullptr; };

protected:
  std::shared_ptr<PhysicalDevice> physicalDevice;
  std::shared_ptr<LogicalDevice> logicalDevice;

  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

  std::vector<std::unique_ptr<ShaderModule>> shaderModules;
};



#endif //GRAPHICSPIPELINE_H
