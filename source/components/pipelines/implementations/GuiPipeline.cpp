#include "GuiPipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "../../RenderPass.h"
#include "../../ImGuiInstance.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include <imgui.h>

namespace vke {

GuiPipeline::GuiPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         std::shared_ptr<RenderPass> renderPass)
  : GraphicsPipeline(logicalDevice)
{
  const GraphicsPipelineOptions graphicsPipelineOptions {
    .shaders {
      .vertexShader = "assets/shaders/ui.vert.spv",
      .fragmentShader = "assets/shaders/ui.frag.spv"
    },
    .states {
      .colorBlendState = GraphicsPipelineStates::colorBlendState,
      .depthStencilState = GraphicsPipelineStates::depthStencilState,
      .dynamicState = GraphicsPipelineStates::dynamicState,
      .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleList,
      .multisampleState = GraphicsPipelineStates::getMultsampleState(m_logicalDevice),
      .rasterizationState = GraphicsPipelineStates::rasterizationStateCullBack,
      .vertexInputState = GraphicsPipelineStates::vertexInputStateVertex,
      .viewportState = GraphicsPipelineStates::viewportState
    },
    .renderPass = renderPass
  };

  createPipeline(graphicsPipelineOptions);
}

void GuiPipeline::render(const RenderInfo* renderInfo)
{
  GraphicsPipeline::render(renderInfo, nullptr);

  ImGui::Render();
  ImGuiInstance::renderDrawData(renderInfo->commandBuffer);
}

} // namespace vke