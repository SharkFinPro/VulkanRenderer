#include "BendyPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "descriptorSets/DescriptorSet.h"
#include "descriptorSets/LayoutBindings.h"
#include "../RenderPass.h"
#include "../../components/textures/Texture2D.h"
#include "../../components/core/commandBuffer/CommandBuffer.h"
#include "../../objects/UniformBuffer.h"

BendyPipeline::BendyPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             const VkCommandPool& commandPool,
                             VkDescriptorPool descriptorPool)
  : GraphicsPipeline(logicalDevice), m_previousTime(std::chrono::steady_clock::now())
{
  definePushConstants();

  createUniforms(commandPool);

  createDescriptorSets(descriptorPool);

  createPipeline(renderPass->getRenderPass());
}

void BendyPipeline::render(const RenderInfo* renderInfo)
{
  GraphicsPipeline::render(renderInfo, nullptr);

  for (const auto bendyPlant : m_bendyPlantsToRender)
  {
    BendyPlantInfo bendyPlantInfo {
      .model = glm::translate(glm::mat4(1.0f), bendyPlant.position),
      .leafLength = bendyPlant.leafLength,
      .pitch = bendyPlant.pitch,
      .bendStrength = bendyPlant.bendStrength
    };

    renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                             sizeof(BendyPlantInfo), &bendyPlantInfo);

    renderInfo->commandBuffer->draw(bendyPlant.leafLength * 2 * 4 + 2, bendyPlant.numFins, 0, 0);
  }
}

void BendyPipeline::renderBendyPlant(const BendyPlant& bendyPlant)
{
  m_bendyPlantsToRender.push_back(bendyPlant);
}

void BendyPipeline::clearBendyPlantsToRender()
{
  m_bendyPlantsToRender.clear();
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
  defineColorBlendState(GraphicsPipelineStates::colorBlendStateBendy);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleStrip);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleStateAlpha(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateRaw);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void BendyPipeline::definePushConstants()
{
  constexpr VkPushConstantRange bendyPlantPushConstant {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .offset = 0,
    .size = sizeof(BendyPlantInfo)
  };

  definePushConstantRange(bendyPlantPushConstant);
}

void BendyPipeline::createUniforms(const VkCommandPool& commandPool)
{
  m_transformUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(VPTransformUniform));

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
  const VPTransformUniform transformUBO {
    .vp = renderInfo->projectionMatrix * renderInfo->viewMatrix
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
