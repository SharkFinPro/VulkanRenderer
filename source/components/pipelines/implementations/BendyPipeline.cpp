#include "BendyPipeline.h"
#include "common/GraphicsPipelineStates.h"
#include "../descriptorSets/DescriptorSet.h"
#include "../uniformBuffers/UniformBuffer.h"
#include "../../assets/textures/Texture2D.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../renderingManager/renderer3D/Renderer3D.h"

namespace {

  constexpr vk::DescriptorSetLayoutBinding MVPTransformLayout {
    .binding = 0,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eVertex
  };

  constexpr vk::DescriptorSetLayoutBinding bendyLayout {
    .binding = 1,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eVertex
  };

  constexpr vk::DescriptorSetLayoutBinding textureLayout {
    .binding = 2,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .descriptorCount = 1,
    .stageFlags = vk::ShaderStageFlagBits::eFragment
  };

  inline std::vector bendyLayoutBindings {
    MVPTransformLayout,
    bendyLayout,
    textureLayout
  };

}

namespace {

  using VPTransformUniform = glm::mat4;

  struct BendyPlantInfo {
    glm::mat4 model;
    int leafLength;
    float pitch;
    float bendStrength;
  };

}

namespace vke {

  BendyPipeline::BendyPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                               const vk::raii::CommandPool& commandPool,
                               const vk::DescriptorPool descriptorPool,
                               const std::shared_ptr<DescriptorSet>& lightingDescriptorSet)
    : m_lightingDescriptorSet(lightingDescriptorSet), m_previousTime(std::chrono::steady_clock::now())
  {
    createUniforms(logicalDevice, commandPool);

    createDescriptorSets(logicalDevice, descriptorPool);

    const GraphicsPipelineOptions graphicsPipelineOptions {
      .shaders {
        .vertexShader = "assets/shaders/Bendy.vert.spv",
        .fragmentShader = "assets/shaders/Bendy.frag.spv"
      },
      .states {
        .colorBlendState = gps::colorBlendStateBendy,
        .depthStencilState = gps::depthStencilState,
        .dynamicState = gps::dynamicState,
        .inputAssemblyState = gps::inputAssemblyStateTriangleStrip,
        .multisampleState = gps::getMultsampleStateAlpha(logicalDevice),
        .rasterizationState = gps::rasterizationStateNoCull,
        .vertexInputState = gps::vertexInputStateRaw,
        .viewportState = gps::viewportState
      },
      .pushConstantRanges {
        {
          .stageFlags = vk::ShaderStageFlagBits::eVertex,
          .offset = 0,
          .size = sizeof(BendyPlantInfo)
        }
      },
      .descriptorSetLayouts {
        m_BendyPipelineDescriptorSet->getDescriptorSetLayout(),
        m_lightingDescriptorSet->getDescriptorSetLayout(),
      }
    };

    createPipeline(logicalDevice, graphicsPipelineOptions);
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

      renderInfo->commandBuffer->pushConstants<BendyPlantInfo>(
        m_pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        0,
        bendyPlantInfo
      );

      renderInfo->commandBuffer->draw(leafLength * 2 * 4 + 2, numFins, 0, 0);
    }
  }

  void BendyPipeline::createUniforms(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                     const vk::raii::CommandPool& commandPool)
  {
    m_transformUniform = std::make_shared<UniformBuffer>(logicalDevice, sizeof(VPTransformUniform));

    m_timeUniform = std::make_shared<UniformBuffer>(logicalDevice, sizeof(float));

    m_texture = std::make_shared<Texture2D>(logicalDevice, commandPool, "assets/bendy/leaf.png", vk::SamplerAddressMode::eClampToEdge);
  }

  void BendyPipeline::createDescriptorSets(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                           vk::DescriptorPool descriptorPool)
  {
    m_BendyPipelineDescriptorSet = std::make_shared<DescriptorSet>(logicalDevice, descriptorPool, bendyLayoutBindings);
    m_BendyPipelineDescriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<vk::WriteDescriptorSet> descriptorWrites{{
        m_transformUniform->getDescriptorSet(0, descriptorSet, frame),
        m_timeUniform->getDescriptorSet(1, descriptorSet, frame),
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

    m_time += dt;

    m_timeUniform->update(renderInfo->currentFrame, &m_time);
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