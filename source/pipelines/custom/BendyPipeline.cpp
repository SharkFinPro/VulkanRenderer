#include "BendyPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "descriptorSets/DescriptorSet.h"
#include "descriptorSets/LayoutBindings.h"
#include "../RenderPass.h"
#include "../../components/textures/Texture2D.h"
#include "../../core/commandBuffer/CommandBuffer.h"
#include "../../objects/UniformBuffer.h"
#include <imgui.h>

BendyPipeline::BendyPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const VkCommandPool& commandPool,
                             VkDescriptorPool descriptorPool)
  : GraphicsPipeline(logicalDevice), m_previousTime(std::chrono::steady_clock::now())
{
  createUniforms(commandPool);

  createDescriptorSets(descriptorPool);

  createPipeline(renderPass->getRenderPass());
}

void BendyPipeline::render(const RenderInfo* renderInfo)
{
  static int numFins = 21;

  ImGui::Begin("Vertices");
  ImGui::SliderInt("# Fins", &numFins, 0, 100);
  ImGui::SliderInt("Leaf Length", &m_bendyUBO.leafLength, 0, 20);
  ImGui::SliderFloat3("Position", &m_position.x, -10, 10);
  ImGui::SliderFloat("Pitch", &m_bendyUBO.pitch, -90, 90);
  ImGui::SliderFloat("Bend Strength", &m_bendyUBO.bendStrength, -1, 1);
  ImGui::End();

  GraphicsPipeline::render(renderInfo, nullptr);

  renderInfo->commandBuffer->draw(m_bendyUBO.leafLength * 2 + 2, numFins, 0, 0);
}

void BendyPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Bendy.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Bendy.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void BendyPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_BendyPipelineDescriptorSet->getDescriptorSetLayout());
}

void BendyPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendStateDots);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilStateNone);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleStrip);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateRaw);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void BendyPipeline::createUniforms(const VkCommandPool& commandPool)
{
  m_transformUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(MVPTransformUniform));

  m_bendyUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(BendyUniform));

  m_texture = std::make_shared<Texture2D>(m_logicalDevice, commandPool, "assets/bendy/leaf.png", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void BendyPipeline::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_BendyPipelineDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::bendyLayoutBindings);
  m_BendyPipelineDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_transformUniform->getDescriptorSet(0, descriptorSet, frame),
      m_bendyUniform->getDescriptorSet(1, descriptorSet, frame),
      m_texture->getDescriptorSet(2, descriptorSet)
    }};

    return descriptorWrites;
  });
}

void BendyPipeline::updateUniformVariables(const RenderInfo *renderInfo)
{
  const MVPTransformUniform transformUBO {
    .mvp = renderInfo->projectionMatrix * renderInfo->viewMatrix * glm::translate(glm::mat4(1.0f), m_position)
  };

  m_transformUniform->update(renderInfo->currentFrame, &transformUBO);

  const auto currentTime = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(currentTime - m_previousTime).count();
  m_previousTime = currentTime;

  m_bendyUBO.time += dt;

  m_bendyUniform->update(renderInfo->currentFrame, &m_bendyUBO);
}

void BendyPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_BendyPipelineDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
