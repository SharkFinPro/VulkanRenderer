#ifndef LINEPIPELINE_H
#define LINEPIPELINE_H

#include "../GraphicsPipeline.h"
#include "../LineVertex.h"
#include <vector>
#include <memory>

class RenderPass;
class UniformBuffer;

class LinePipeline final : public GraphicsPipeline {
public:
  LinePipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                const std::shared_ptr<LogicalDevice>& logicalDevice,
                const std::shared_ptr<RenderPass>& renderPass,
                VkDescriptorPool descriptorPool);

  ~LinePipeline() override;

  void render(const RenderInfo* renderInfo, const VkCommandPool& commandPool, const std::vector<LineVertex>& vertices);

private:
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkDescriptorSetLayout lineDescriptorSetLayout = VK_NULL_HANDLE;

  std::unique_ptr<UniformBuffer> transformUniform;

  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  size_t maxVertexBufferSize = sizeof(LineVertex) * 20'000;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createLineDescriptorSetLayout();

  void createDescriptorSets();

  void createUniforms();

  void updateUniformVariables(const RenderInfo *renderInfo) override;

  void bindDescriptorSet(const RenderInfo *renderInfo) override;

  void createVertexBuffer();
};



#endif //LINEPIPELINE_H
