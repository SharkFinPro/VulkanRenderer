#ifndef TEXTUREDPLANE_H
#define TEXTUREDPLANE_H

#include "../GraphicsPipeline.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;

class TexturedPlane final : public GraphicsPipeline {
public:
  TexturedPlane(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass, VkDescriptorSetLayout objectDescriptorSetLayout);
  ~TexturedPlane() override;

  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, glm::vec3 viewPosition,
              const glm::mat4& viewMatrix, VkExtent2D swapChainExtent,
              const std::vector<std::shared_ptr<RenderObject>>& objects) const;

private:
  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> cameraUniform;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorPool();

  void createDescriptorSets();

  void createUniforms();
};



#endif //TEXTUREDPLANE_H
