#ifndef VKE_GRAPHICSPIPELINE_H
#define VKE_GRAPHICSPIPELINE_H

#include "Pipeline.h"
#include "shaderModules/ShaderModule.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <vector>

namespace vke {

  class CommandBuffer;
  class RenderPass;

  struct RenderInfo {
    std::shared_ptr<CommandBuffer> commandBuffer;
    uint32_t currentFrame;
    glm::vec3 viewPosition;
    const glm::mat4& viewMatrix;
    vk::Extent2D extent;

    mutable glm::mat4 projectionMatrix;
    mutable bool shouldCreateProjectionMatrix = true;

    [[nodiscard]] glm::mat4& getProjectionMatrix() const
    {
      if (shouldCreateProjectionMatrix)
      {
        projectionMatrix = glm::perspective(
          glm::radians(45.0f),
          static_cast<float>(extent.width) / static_cast<float>(extent.height),
          0.1f,
          1000.0f
        );

        projectionMatrix[1][1] *= -1;
      }

      return projectionMatrix;
    }
  };

  struct GraphicsPipelineOptions {
    struct {
      std::string vertexShader;
      std::string geometryShader;
      std::string tesselationControlShader;
      std::string tesselationEvaluationShader;
      std::string fragmentShader;

      [[nodiscard]] std::vector<ShaderModule> getShaderModules(const std::shared_ptr<LogicalDevice>& logicalDevice) const
      {
        std::vector<ShaderModule> shaderModules;

        if (!vertexShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, vertexShader.c_str(), vk::ShaderStageFlagBits::eVertex);
        }

        if (!fragmentShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, fragmentShader.c_str(), vk::ShaderStageFlagBits::eFragment);
        }

        if (!geometryShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, geometryShader.c_str(), vk::ShaderStageFlagBits::eGeometry);
        }

        if (!tesselationControlShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, tesselationControlShader.c_str(), vk::ShaderStageFlagBits::eTessellationControl);
        }

        if (!tesselationEvaluationShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, tesselationEvaluationShader.c_str(), vk::ShaderStageFlagBits::eTessellationEvaluation);
        }

        return shaderModules;
      }

      static std::vector<vk::PipelineShaderStageCreateInfo> getShaderStages(const std::vector<ShaderModule>& shaderModules)
      {
        std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        for (const auto& shaderModule : shaderModules)
        {
          pipelineShaderStageCreateInfos.push_back(shaderModule.getShaderStageCreateInfo());
        }

        return pipelineShaderStageCreateInfos;
      }
    } shaders;

    struct {
      vk::PipelineColorBlendStateCreateInfo colorBlendState{};
      vk::PipelineDepthStencilStateCreateInfo depthStencilState{};
      vk::PipelineDynamicStateCreateInfo dynamicState{};
      vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{};
      vk::PipelineMultisampleStateCreateInfo multisampleState{};
      vk::PipelineRasterizationStateCreateInfo rasterizationState{};
      vk::PipelineTessellationStateCreateInfo tessellationState{};
      vk::PipelineVertexInputStateCreateInfo vertexInputState{};
      vk::PipelineViewportStateCreateInfo viewportState{};
    } states;

    std::vector<vk::PushConstantRange> pushConstantRanges;

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

    const std::shared_ptr<RenderPass>& renderPass;

    vk::Format colorFormat = vk::Format::eR8G8B8A8Unorm;

    bool renderToCubeMap = false;
  };

  class GraphicsPipeline : public Pipeline {
  public:
    GraphicsPipeline() = default;

    GraphicsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                     const GraphicsPipelineOptions& graphicsPipelineOptions);

    void bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                           vk::DescriptorSet descriptorSet,
                           uint32_t location) const;

  protected:
    void createPipelineLayout(const std::shared_ptr<LogicalDevice>& logicalDevice,
                              const GraphicsPipelineOptions& graphicsPipelineOptions);

    void createPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                        const GraphicsPipelineOptions& graphicsPipelineOptions);
  };

} // namespace vke

#endif //VKE_GRAPHICSPIPELINE_H