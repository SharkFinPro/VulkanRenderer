#ifndef CUBEMAPPIPELINE_H
#define CUBEMAPPIPELINE_H

#include "Uniforms.h"
#include "../GraphicsPipeline.h"
#include <glm/glm.hpp>

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Noise3DTexture;
class CubeMapTexture;

class CubeMapPipeline final : public GraphicsPipeline {
public:
  CubeMapPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                  const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass, const VkCommandPool& commandPool);
  ~CubeMapPipeline() override;

  VkDescriptorSetLayout& getLayout();

  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, glm::vec3 viewPosition,
              const glm::mat4& viewMatrix, VkExtent2D swapChainExtent,
              const std::vector<std::shared_ptr<RenderObject>>& objects);

private:
  CubeMapUniform cubeMapUBO {
    .mix = 0,
    .refractionIndex = 1.4,
    .whiteMix = 0.2
  };

  NoiseOptionsUniform noiseOptionsUBO {
    .amplitude = 0.0f,
    .frequency = 0.1f
  };

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> cameraUniform;
  std::unique_ptr<UniformBuffer> cubeMapUniform;
  std::unique_ptr<UniformBuffer> noiseOptionsUniform;
  std::unique_ptr<Noise3DTexture> noiseTexture;

  std::unique_ptr<CubeMapTexture> reflectUnit;
  std::unique_ptr<CubeMapTexture> refractUnit;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createDescriptorSetLayouts();

  void createGlobalDescriptorSetLayout();
  void createObjectDescriptorSetLayout();

  void createDescriptorPool();

  void createDescriptorSets();

  void createUniforms(const VkCommandPool& commandPool);
};



#endif //CUBEMAPPIPELINE_H
