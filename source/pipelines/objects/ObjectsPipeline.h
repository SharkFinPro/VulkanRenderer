#ifndef VULKANPROJECT_OBJECTSPIPELINE_H
#define VULKANPROJECT_OBJECTSPIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <array>

#include "Uniforms.h"
#include "../GraphicsPipeline.h"

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;

class ObjectsPipeline final : public GraphicsPipeline {
public:
  ObjectsPipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass);
  ~ObjectsPipeline() override;

  VkDescriptorSetLayout& getLayout();

  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, const std::shared_ptr<Camera>& camera,
              VkExtent2D swapChainExtent);

  void insertRenderObject(const std::shared_ptr<RenderObject>& renderObject);

  void createLight(glm::vec3 position, glm::vec3 color, float ambient, float diffuse, float specular = 1.0f);

private:
  std::vector<std::shared_ptr<RenderObject>> renderObjects;

  VkDescriptorSetLayout globalDescriptorSetLayout;
  VkDescriptorSetLayout objectDescriptorSetLayout;

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> lightMetadataUniform;
  std::unique_ptr<UniformBuffer> lightsUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;

  size_t lightsUniformBufferSize;

  std::vector<LightUniform> lights;

  VkPipelineColorBlendAttachmentState colorBlendAttachment;

  std::array<VkDynamicState, 2> dynamicStates;

  VkVertexInputBindingDescription vertexBindingDescription;
  std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions;

  void updateLightUniforms(uint32_t currentFrame);

  void renderLightsGui();

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
};


#endif //VULKANPROJECT_OBJECTSPIPELINE_H
