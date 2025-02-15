#ifndef ELLIPTICALDOTS_H
#define ELLIPTICALDOTS_H

#include "Uniforms.h"
#include "../GraphicsPipeline.h"
#include <glm/glm.hpp>

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Light;

class EllipticalDots final : public GraphicsPipeline {
public:
  EllipticalDots(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                 const std::shared_ptr<LogicalDevice>& logicalDevice,
                 const std::shared_ptr<RenderPass>& renderPass, VkDescriptorSetLayout objectDescriptorSetLayout);
  ~EllipticalDots() override;

  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, glm::vec3 viewPosition,
              const glm::mat4& viewMatrix, VkExtent2D swapChainExtent,
              const std::vector<std::shared_ptr<Light>>& lights,
              const std::vector<std::shared_ptr<RenderObject>>& objects);

private:
  EllipticalDotsUniform ellipticalDotsUBO {
    .shininess = 10.0f,
    .sDiameter = 0.025f,
    .tDiameter = 0.025f,
    .blendFactor = 0.0f
  };

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> lightMetadataUniform;
  std::unique_ptr<UniformBuffer> lightsUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;
  std::unique_ptr<UniformBuffer> ellipticalDotsUniform;

  int prevNumLights = 0;

  size_t lightsUniformBufferSize = 0;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorPool();

  void createDescriptorSets();

  void createUniforms();

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);
};



#endif //ELLIPTICALDOTS_H
