#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

#include "ShaderModule.h"
#include "../components/PhysicalDevice.h"
#include "../components/LogicalDevice.h"

#include "Pipeline.h"

class GraphicsPipeline : public Pipeline {
public:
  GraphicsPipeline(const std::shared_ptr<PhysicalDevice> &physicalDevice, const std::shared_ptr<LogicalDevice> &logicalDevice);

  ~GraphicsPipeline() override = default;

protected:
  void createShader(const char* filename, VkShaderStageFlagBits stage);

  virtual void loadGraphicsShaders() = 0;

  void loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
  virtual void loadGraphicsDescriptorSetLayouts() {};

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
  std::vector<std::unique_ptr<ShaderModule>> shaderModules;
};



#endif //GRAPHICSPIPELINE_H
