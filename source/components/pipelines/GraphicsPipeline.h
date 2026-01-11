#ifndef VKE_GRAPHICSPIPELINE_H
#define VKE_GRAPHICSPIPELINE_H

#include "Pipeline.h"
#include "shaderModules/ShaderModule.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <vector>

namespace vke {

  class CommandBuffer;
  class RenderObject;
  class RenderPass;

  struct RenderInfo {
    std::shared_ptr<CommandBuffer> commandBuffer;
    uint32_t currentFrame;
    glm::vec3 viewPosition;
    const glm::mat4& viewMatrix;
    VkExtent2D extent;

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
          shaderModules.emplace_back(logicalDevice, vertexShader.c_str(), VK_SHADER_STAGE_VERTEX_BIT);
        }

        if (!fragmentShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, fragmentShader.c_str(), VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        if (!geometryShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, geometryShader.c_str(), VK_SHADER_STAGE_GEOMETRY_BIT);
        }

        if (!tesselationControlShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, tesselationControlShader.c_str(), VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
        }

        if (!tesselationEvaluationShader.empty())
        {
          shaderModules.emplace_back(logicalDevice, tesselationEvaluationShader.c_str(), VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
        }

        return std::move(shaderModules);
      }

      static std::vector<VkPipelineShaderStageCreateInfo> getShaderStages(const std::vector<ShaderModule>& shaderModules)
      {
        std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

        for (const auto& shaderModule : shaderModules)
        {
          pipelineShaderStageCreateInfos.push_back(shaderModule.getShaderStageCreateInfo());
        }

        return std::move(pipelineShaderStageCreateInfos);
      }
    } shaders;

    struct {
      VkPipelineColorBlendStateCreateInfo colorBlendState{};
      VkPipelineDepthStencilStateCreateInfo depthStencilState{};
      VkPipelineDynamicStateCreateInfo dynamicState{};
      VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
      VkPipelineMultisampleStateCreateInfo multisampleState{};
      VkPipelineRasterizationStateCreateInfo rasterizationState{};
      VkPipelineTessellationStateCreateInfo tessellationState{};
      VkPipelineVertexInputStateCreateInfo vertexInputState{};
      VkPipelineViewportStateCreateInfo viewportState{};
    } states;

    std::vector<VkPushConstantRange> pushConstantRanges;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    const std::shared_ptr<RenderPass>& renderPass;

    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;

    bool renderToCubeMap = false;
  };

  class GraphicsPipeline : public Pipeline {
  public:
    explicit GraphicsPipeline(std::shared_ptr<LogicalDevice> logicalDevice);

    GraphicsPipeline(std::shared_ptr<LogicalDevice> logicalDevice,
                     const GraphicsPipelineOptions& graphicsPipelineOptions);

    virtual void render(const RenderInfo* renderInfo,
                        const std::vector<std::shared_ptr<RenderObject>>* objects);

    void bind(const std::shared_ptr<CommandBuffer>& commandBuffer) const;

    void bindDescriptorSet(const std::shared_ptr<CommandBuffer>& commandBuffer,
                           VkDescriptorSet descriptorSet,
                           uint32_t location) const;

  protected:
    void createPipelineLayout(const GraphicsPipelineOptions& graphicsPipelineOptions);

    void createPipeline(const GraphicsPipelineOptions& graphicsPipelineOptions);

    virtual void updateUniformVariables(const RenderInfo* renderInfo) {}

    virtual void bindDescriptorSet(const RenderInfo* renderInfo) {}
  };

} // namespace vke

#endif //VKE_GRAPHICSPIPELINE_H
