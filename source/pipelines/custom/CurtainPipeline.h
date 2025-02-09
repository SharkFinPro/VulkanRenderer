#ifndef CURTAINPIPELINE_H
#define CURTAINPIPELINE_H

#include "Uniforms.h"
#include "../GraphicsPipeline.h"
#include <array>
#include <glm/glm.hpp>

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Light;

class CurtainPipeline final : public GraphicsPipeline {
public:
  CurtainPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                 const std::shared_ptr<LogicalDevice>& logicalDevice,
                 const std::shared_ptr<RenderPass>& renderPass);
  ~CurtainPipeline() override;

  VkDescriptorSetLayout& getLayout();

  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, glm::vec3 viewPosition,
              const glm::mat4& viewMatrix, VkExtent2D swapChainExtent,
              const std::vector<std::shared_ptr<Light>>& lights,
              const std::vector<std::shared_ptr<RenderObject>>& objects);

private:
  CurtainUniform curtainUBO {
    .amplitude = 0.1,
    .period = 1,
    .shininess = 10
  };

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> lightMetadataUniform;
  std::unique_ptr<UniformBuffer> lightsUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;
  std::unique_ptr<UniformBuffer> curtainUniform;

  int prevNumLights = 0;

  size_t lightsUniformBufferSize;

  VkPipelineColorBlendAttachmentState colorBlendAttachment;

  std::array<VkDynamicState, 2> dynamicStates;

  VkVertexInputBindingDescription vertexBindingDescription;
  std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  std::unique_ptr<VkPipelineColorBlendStateCreateInfo> defineColorBlendState() override;
  std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> defineDepthStencilState() override;
  std::unique_ptr<VkPipelineDynamicStateCreateInfo> defineDynamicState() override;
  std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> defineInputAssemblyState() override;
  std::unique_ptr<VkPipelineMultisampleStateCreateInfo> defineMultisampleState() override;
  std::unique_ptr<VkPipelineRasterizationStateCreateInfo> defineRasterizationState() override;
  std::unique_ptr<VkPipelineVertexInputStateCreateInfo> defineVertexInputState() override;
  std::unique_ptr<VkPipelineViewportStateCreateInfo> defineViewportState() override;

  void createDescriptorSetLayouts();

  void createGlobalDescriptorSetLayout();
  void createObjectDescriptorSetLayout();

  void createDescriptorPool();

  void createDescriptorSets();

  void createUniforms();

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);
};

#endif //CURTAINPIPELINE_H
