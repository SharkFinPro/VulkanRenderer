#ifndef MAGNIFYWHIRLMOSAIC_H
#define MAGNIFYWHIRLMOSAIC_H

#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class LogicalDevice;
class PhysicalDevice;
class RenderPass;
class RenderObject;
class UniformBuffer;

class MagnifyWhirlMosaicPipeline final : public GraphicsPipeline {
public:
  MagnifyWhirlMosaicPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                             const std::shared_ptr<LogicalDevice>& logicalDevice,
                             const std::shared_ptr<RenderPass>& renderPass,
                             VkDescriptorPool descriptorPool,
                             VkDescriptorSetLayout objectDescriptorSetLayout);

  ~MagnifyWhirlMosaicPipeline() override;

  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, glm::vec3 viewPosition,
              const glm::mat4& viewMatrix, VkExtent2D swapChainExtent,
              const std::vector<std::shared_ptr<RenderObject>>& objects);

private:
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> cameraUniform;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createGlobalDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms();
};



#endif //MAGNIFYWHIRLMOSAIC_H
