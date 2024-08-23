#ifndef TORUSDISPLAYCOLORPIPELINE_H
#define TORUSDISPLAYCOLORPIPELINE_H

#include <vulkan/vulkan.h>
#include <array>

#include "../../components/PhysicalDevice.h"
#include "../../components/LogicalDevice.h"

#include "../GraphicsPipeline.h"
#include "../RenderPass.h"


class TorusDisplayColorPipeline final : public GraphicsPipeline {
public:
  TorusDisplayColorPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                            const std::shared_ptr<LogicalDevice>& logicalDevice,
                            const std::shared_ptr<RenderPass>& renderPass);

private:
  void loadGraphicsShaders() override;

  std::unique_ptr<VkPipelineColorBlendStateCreateInfo> defineColorBlendState() override;
  std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> defineDepthStencilState() override;
  std::unique_ptr<VkPipelineDynamicStateCreateInfo> defineDynamicState() override;
  std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> defineInputAssemblyState() override;
  std::unique_ptr<VkPipelineMultisampleStateCreateInfo> defineMultisampleState() override;
  std::unique_ptr<VkPipelineRasterizationStateCreateInfo> defineRasterizationState() override;
  std::unique_ptr<VkPipelineVertexInputStateCreateInfo> defineVertexInputState() override;
  std::unique_ptr<VkPipelineViewportStateCreateInfo> defineViewportState() override;

private:
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  std::array<VkDynamicState, 2> dynamicStates;
  VkVertexInputBindingDescription vertexBindingDescription;
  std::array<VkVertexInputAttributeDescription, 2> vertexAttributeDescriptions;
};



#endif //TORUSDISPLAYCOLORPIPELINE_H
