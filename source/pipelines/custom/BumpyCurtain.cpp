#include "BumpyCurtain.h"
#include "config/GraphicsPipelineStates.h"
#include "descriptorSets/DescriptorSet.h"
#include "descriptorSets/LayoutBindings.h"
#include "../RenderPass.h"
#include "../../components/textures/Texture3D.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/Light.h"
#include "../../objects/UniformBuffer.h"
#include <imgui.h>

BumpyCurtain::BumpyCurtain(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const std::shared_ptr<RenderPass>& renderPass,
                           const VkCommandPool& commandPool,
                           const VkDescriptorPool descriptorPool,
                           const VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice),
    m_objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms(commandPool);

  createDescriptorSets(descriptorPool);

  createPipeline(renderPass->getRenderPass());
}

void BumpyCurtain::displayGui()
{
  ImGui::Begin("Bumpy Curtain");

  ImGui::SliderFloat("Amplitude", &m_curtainUBO.amplitude, 0.001f, 3.0f);
  ImGui::SliderFloat("Period", &m_curtainUBO.period, 0.1f, 10.0f);
  ImGui::SliderFloat("Shininess", &m_curtainUBO.shininess, 1.0f, 100.0f);

  ImGui::Separator();

  ImGui::SliderFloat("Noise Amplitude", &m_noiseOptionsUBO.amplitude, 0.0f, 10.0f);
  ImGui::SliderFloat("Noise Frequency", &m_noiseOptionsUBO.frequency, 0.1f, 10.0f);

  ImGui::End();
}

void BumpyCurtain::loadGraphicsShaders()
{
  createShader("assets/shaders/Curtain.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/BumpyCurtain.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void BumpyCurtain::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(m_bumpyCurtainDescriptorSet->getDescriptorSetLayout());
  loadDescriptorSetLayout(m_objectDescriptorSetLayout);
  loadDescriptorSetLayout(m_lightingDescriptorSet->getDescriptorSetLayout());
}

void BumpyCurtain::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateNoCull);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void BumpyCurtain::createUniforms(const VkCommandPool& commandPool)
{
  m_lightMetadataUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightMetadataUniform));

  m_lightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightUniform));

  m_cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));

  m_curtainUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CurtainUniform));

  m_noiseOptionsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(NoiseOptionsUniform));

  m_noiseTexture = std::make_shared<Texture3D>(m_logicalDevice, commandPool, "assets/noise/noise3d.064.tex",
                                             VK_SAMPLER_ADDRESS_MODE_REPEAT);
}

void BumpyCurtain::createDescriptorSets(VkDescriptorPool descriptorPool)
{
  m_lightingDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::lightingLayoutBindings);
  m_lightingDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_lightMetadataUniform->getDescriptorSet(2, descriptorSet, frame),
      m_cameraUniform->getDescriptorSet(3, descriptorSet, frame)
    }};

    return descriptorWrites;
  });

  m_bumpyCurtainDescriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, LayoutBindings::bumpyCurtainLayoutBindings);
  m_bumpyCurtainDescriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      m_curtainUniform->getDescriptorSet(4, descriptorSet, frame),
      m_noiseOptionsUniform->getDescriptorSet(6, descriptorSet, frame),
      m_noiseTexture->getDescriptorSet(7, descriptorSet)
    }};

    return descriptorWrites;
  });
}

void BumpyCurtain::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
{
  if (lights.empty())
  {
    return;
  }

  if (m_prevNumLights != lights.size())
  {
    m_logicalDevice->waitIdle();

    const LightMetadataUniform lightMetadataUBO {
      .numLights = static_cast<int>(lights.size())
    };

    m_lightsUniform.reset();

    m_lightsUniformBufferSize = sizeof(LightUniform) * lights.size();

    m_lightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, m_lightsUniformBufferSize);

    m_lightingDescriptorSet->updateDescriptorSets([this, lightMetadataUBO](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      m_lightMetadataUniform->update(frame, &lightMetadataUBO);

      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_lightsUniform->getDescriptorSet(5, descriptorSet, frame)
      }};

      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      return descriptorWrites;
    });

    m_prevNumLights = static_cast<int>(lights.size());
  }

  std::vector<LightUniform> lightUniforms;
  lightUniforms.resize(lights.size());
  for (int i = 0; i < lights.size(); i++)
  {
    lightUniforms[i] = lights[i]->getUniform();
  }

  m_lightsUniform->update(currentFrame, lightUniforms.data());
}

void BumpyCurtain::updateUniformVariables(const RenderInfo* renderInfo)
{
  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);

  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  m_cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  m_curtainUniform->update(renderInfo->currentFrame, &m_curtainUBO);

  m_noiseOptionsUniform->update(renderInfo->currentFrame, &m_noiseOptionsUBO);
}

void BumpyCurtain::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &m_bumpyCurtainDescriptorSet->getDescriptorSet(renderInfo->currentFrame));

  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
                                                &m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
