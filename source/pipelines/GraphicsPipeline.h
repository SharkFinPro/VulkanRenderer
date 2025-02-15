#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include "Pipeline.h"
#include "ShaderModule.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class GraphicsPipeline : public Pipeline {
public:
  GraphicsPipeline(const std::shared_ptr<PhysicalDevice> &physicalDevice, const std::shared_ptr<LogicalDevice> &logicalDevice);

protected:
  std::vector<std::unique_ptr<ShaderModule>> shaderModules;

  void createShader(const char* filename, VkShaderStageFlagBits stage);

  virtual void loadGraphicsShaders() = 0;

  void loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
  virtual void loadGraphicsDescriptorSetLayouts() {};

  void createPipelineLayout();

  void createPipeline(const VkRenderPass& renderPass);

  void defineColorBlendState(const VkPipelineColorBlendStateCreateInfo& state);

  void defineDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& state);

  void defineDynamicState(const VkPipelineDynamicStateCreateInfo& state);

  void defineInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& state);

  void defineMultisampleState(const VkPipelineMultisampleStateCreateInfo& state);

  void defineRasterizationState(const VkPipelineRasterizationStateCreateInfo& state);

  void defineTessellationState(const VkPipelineTessellationStateCreateInfo& state);

  void defineVertexInputState(const VkPipelineVertexInputStateCreateInfo& state);

  void defineViewportState(const VkPipelineViewportStateCreateInfo& state);

  virtual void defineStates() = 0;

private:
  std::unique_ptr<VkPipelineColorBlendStateCreateInfo> colorBlendState{};
  std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> depthStencilState{};
  std::unique_ptr<VkPipelineDynamicStateCreateInfo> dynamicState{};
  std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> inputAssemblyState{};
  std::unique_ptr<VkPipelineMultisampleStateCreateInfo> multisampleState{};
  std::unique_ptr<VkPipelineRasterizationStateCreateInfo> rasterizationState{};
  std::unique_ptr<VkPipelineTessellationStateCreateInfo> tessellationState{};
  std::unique_ptr<VkPipelineVertexInputStateCreateInfo> vertexInputState{};
  std::unique_ptr<VkPipelineViewportStateCreateInfo> viewportState{};
};



#endif //GRAPHICSPIPELINE_H
