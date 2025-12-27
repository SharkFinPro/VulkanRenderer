#ifndef VKE_BENDYPIPELINE_H
#define VKE_BENDYPIPELINE_H

#include "common/Uniforms.h"
#include "../GraphicsPipeline.h"
#include <glm/vec3.hpp>
#include <chrono>

namespace vke {

  class UniformBuffer;
  class DescriptorSet;
  class RenderPass;
  class Texture2D;

  struct BendyPlant {
    glm::vec3 position = glm::vec3(0.0f);
    int numFins = 21;
    int leafLength = 3;
    float pitch = 77.5;
    float bendStrength = -0.07;
  };

  class BendyPipeline final : public GraphicsPipeline {
  public:
    BendyPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  std::shared_ptr<RenderPass> renderPass,
                  const VkCommandPool& commandPool,
                  VkDescriptorPool descriptorPool,
                  const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

    void render(const RenderInfo* renderInfo);

    void renderBendyPlant(const BendyPlant &bendyPlant);

    void clearBendyPlantsToRender();

  private:
    BendyUniform m_bendyUBO {
      .time = 0
    };

    std::shared_ptr<UniformBuffer> m_transformUniform;
    std::shared_ptr<UniformBuffer> m_bendyUniform;

    std::shared_ptr<DescriptorSet> m_BendyPipelineDescriptorSet;
    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

    std::shared_ptr<Texture2D> m_texture;

    std::chrono::time_point<std::chrono::steady_clock> m_previousTime;

    std::vector<BendyPlant> m_bendyPlantsToRender;

    void createUniforms(const VkCommandPool& commandPool);

    void createDescriptorSets(VkDescriptorPool descriptorPool);

    void updateUniformVariables(const RenderInfo* renderInfo) override;

    void bindDescriptorSet(const RenderInfo* renderInfo) override;
  };

} // namespace vke

#endif //VKE_BENDYPIPELINE_H
