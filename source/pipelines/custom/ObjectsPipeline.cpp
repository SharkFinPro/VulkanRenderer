#include "ObjectsPipeline.h"
#include "config/GraphicsPipelineStates.h"
#include "config/Uniforms.h"
#include "descriptorSets/LightingDescriptorSet.h"
#include "../RenderPass.h"
#include "../../components/Camera.h"
#include "../../core/logicalDevice/LogicalDevice.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"
#include <imgui.h>

ObjectsPipeline::ObjectsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass,
                                 const VkDescriptorPool descriptorPool,
                                 const VkDescriptorSetLayout objectDescriptorSetLayout)
  : GraphicsPipeline(logicalDevice),
    objectDescriptorSetLayout(objectDescriptorSetLayout)
{
  createUniforms();

  lightingDescriptorSet = std::make_shared<LightingDescriptorSet>(m_logicalDevice, descriptorPool);
  lightingDescriptorSet->updateDescriptorSets([this](VkDescriptorSet descriptorSet, const size_t frame)
  {
    std::vector<VkWriteDescriptorSet> descriptorWrites{{
      lightMetadataUniform->getDescriptorSet(2, descriptorSet, frame),
      cameraUniform->getDescriptorSet(3, descriptorSet, frame)
    }};

    return descriptorWrites;
  });

  createPipeline(renderPass->getRenderPass());
}

void ObjectsPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/StandardObject.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/objects.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void ObjectsPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(lightingDescriptorSet->getDescriptorSetLayout());
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

void ObjectsPipeline::defineStates()
{
  defineColorBlendState(GraphicsPipelineStates::colorBlendState);
  defineDepthStencilState(GraphicsPipelineStates::depthStencilState);
  defineDynamicState(GraphicsPipelineStates::dynamicState);
  defineInputAssemblyState(GraphicsPipelineStates::inputAssemblyStateTriangleList);
  defineMultisampleState(GraphicsPipelineStates::getMultsampleState(m_logicalDevice));
  defineRasterizationState(GraphicsPipelineStates::rasterizationStateCullBack);
  defineVertexInputState(GraphicsPipelineStates::vertexInputStateVertex);
  defineViewportState(GraphicsPipelineStates::viewportState);
}

void ObjectsPipeline::createUniforms()
{
  lightMetadataUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightMetadataUniform));

  lightsUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(LightUniform));

  cameraUniform = std::make_shared<UniformBuffer>(m_logicalDevice, sizeof(CameraUniform));
}

void ObjectsPipeline::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
{
  if (lights.empty())
  {
    return;
  }

  if (prevNumLights != lights.size())
  {
    m_logicalDevice->waitIdle();

    const LightMetadataUniform lightMetadataUBO {
      .numLights = static_cast<int>(lights.size())
    };

    lightsUniform.reset();

    lightsUniformBufferSize = sizeof(LightUniform) * lights.size();

    lightsUniform = std::make_unique<UniformBuffer>(m_logicalDevice, lightsUniformBufferSize);

    lightingDescriptorSet->updateDescriptorSets([this, lightMetadataUBO](VkDescriptorSet descriptorSet, const size_t frame)
    {
      lightMetadataUniform->update(frame, &lightMetadataUBO);

      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        lightsUniform->getDescriptorSet(5, descriptorSet, frame)
      }};

      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      return descriptorWrites;
    });

    prevNumLights = static_cast<int>(lights.size());
  }

  std::vector<LightUniform> lightUniforms;
  lightUniforms.resize(lights.size());
  for (int i = 0; i < lights.size(); i++)
  {
    lightUniforms[i] = lights[i]->getUniform();
  }

  lightsUniform->update(currentFrame, lightUniforms.data());
}

void ObjectsPipeline::updateUniformVariables(const RenderInfo* renderInfo)
{
  const CameraUniform cameraUBO {
    .position = renderInfo->viewPosition
  };
  cameraUniform->update(renderInfo->currentFrame, &cameraUBO);

  updateLightUniforms(renderInfo->lights, renderInfo->currentFrame);
}

void ObjectsPipeline::bindDescriptorSet(const RenderInfo* renderInfo)
{
  renderInfo->commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                                &lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame));
}
