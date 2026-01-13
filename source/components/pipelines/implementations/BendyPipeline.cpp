#include "BendyPipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../descriptorSets/LayoutBindings.h"
#include "../uniformBuffers/UniformBuffer.h"
#include "../../assets/textures/Texture2D.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../renderingManager/renderer3D/Renderer3D.h"

namespace vke {

  BendyPipeline::BendyPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                               const std::shared_ptr<RenderPass>& renderPass,
                               const VkCommandPool& commandPool,
                               VkDescriptorPool descriptorPool,
                               const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
    : GraphicsPipeline(std::move(logicalDevice)), m_lightingDescriptorSet(lightingDescriptorSet),
      m_previousTime(std::chrono::steady_clock::now())
  {
    createUniforms(commandPool);

    createDescriptorSets(descriptorPool);

    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/Bendy.vert.spv",
        .fragmentShader = "assets/shaders/Bendy.frag.spv"
      },
      .states {
        .colorBlendState = GraphicsPipelineStates::colorBlendStateBendy,
        .depthStencilState = GraphicsPipelineStates::depthStencilState,
        .dynamicState = GraphicsPipelineStates::dynamicState,
        .inputAssemblyState = GraphicsPipelineStates::inputAssemblyStateTriangleStrip,
        .multisampleState = GraphicsPipelineStates::getMultsampleStateAlpha(m_logicalDevice),
        .rasterizationState = GraphicsPipelineStates::rasterizationStateNoCull,
        .vertexInputState = GraphicsPipelineStates::vertexInputStateRaw,
        .viewportState = GraphicsPipelineStates::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = sizeof(BendyPlantInfo)
        }
      },
      .descriptorSetLayouts {
        m_BendyPipelineDescriptorSet->getDescriptorSetLayout(),
        m_lightingDescriptorSet->getDescriptorSetLayout(),
      },
      .renderPass = renderPass
    };

    createPipeline(graphicsPipelineOptions);
  }

  void BendyPipeline::render(const RenderInfo* renderInfo,
                             const std::vector<BendyPlant>* plants)
  {
    if (!plants)
    {
      return;
    }

    bind(renderInfo->commandBuffer);

    updateUniformVariables(renderInfo);

    bindDescriptorSets(renderInfo);

    for (const auto&[position, numFins, leafLength, pitch, bendStrength] : *plants)
    {
      BendyPlantInfo bendyPlantInfo {
        .model = glm::translate(glm::mat4(1.0f), position),
        .leafLength = leafLength,
        .pitch = pitch,
        .bendStrength = bendStrength
      };

      renderInfo->commandBuffer->pushConstants(m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                               sizeof(BendyPlantInfo), &bendyPlantInfo);

      renderInfo->commandBuffer->draw(leafLength * 2 * 4 + 2, numFins, 0, 0);
    }
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

  void BendyPipeline::updateUniformVariables(const RenderInfo* renderInfo)
  {
    const VPTransformUniform transformUBO = renderInfo->projectionMatrix * renderInfo->viewMatrix;

    m_transformUniform->update(renderInfo->currentFrame, &transformUBO);

    const auto currentTime = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(currentTime - m_previousTime).count();
    m_previousTime = currentTime;

    m_bendyUBO.time += dt;

    m_bendyUniform->update(renderInfo->currentFrame, &m_bendyUBO);
  }

  void BendyPipeline::bindDescriptorSets(const RenderInfo* renderInfo) const
  {
    bindDescriptorSet(
      renderInfo->commandBuffer,
      m_BendyPipelineDescriptorSet->getDescriptorSet(renderInfo->currentFrame),
      0
    );

    bindDescriptorSet(
      renderInfo->commandBuffer,
      m_lightingDescriptorSet->getDescriptorSet(renderInfo->currentFrame),
      1
    );
  }

} // namespace vke