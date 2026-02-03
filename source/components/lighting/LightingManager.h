#ifndef VKE_LIGHTINGMANAGER_H
#define VKE_LIGHTINGMANAGER_H

#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace vke {

  class CommandBuffer;
  class DescriptorSet;
  class Light;
  class LogicalDevice;
  class PipelineManager;
  class Renderer;
  class RenderObject;
  class UniformBuffer;

  class LightingManager {
  public:
    LightingManager(std::shared_ptr<LogicalDevice> logicalDevice,
                    std::shared_ptr<Renderer> renderer);

    ~LightingManager();

    [[nodiscard]] std::shared_ptr<Light> createPointLight(glm::vec3 position,
                                                          glm::vec3 color,
                                                          float ambient,
                                                          float diffuse,
                                                          float specular = 1.0f);

    [[nodiscard]] std::shared_ptr<Light> createSpotLight(glm::vec3 position,
                                                         glm::vec3 color,
                                                         float ambient,
                                                         float diffuse,
                                                         float specular = 1.0f);

    void renderLight(const std::shared_ptr<Light>& light);

    [[nodiscard]] std::shared_ptr<DescriptorSet> getLightingDescriptorSet() const;

    void clearLightsToRender();

    void update(uint32_t currentFrame, glm::vec3 viewPosition);

    void renderShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          const std::vector<std::shared_ptr<RenderObject>>* objects,
                          uint32_t currentFrame) const;

    [[nodiscard]] VkDescriptorSetLayout getPointLightDescriptorSetLayout() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

    std::shared_ptr<UniformBuffer> m_lightMetadataUniform;
    std::shared_ptr<UniformBuffer> m_spotLightsUniform;
    std::shared_ptr<UniformBuffer> m_pointLightsUniform;
    std::shared_ptr<UniformBuffer> m_cameraUniform;

    int m_prevNumPointLights = 0;
    int m_prevNumSpotLights = 0;

    std::vector<std::shared_ptr<Light>> m_pointLightsToRender;
    std::vector<std::shared_ptr<Light>> m_spotLightsToRender;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    std::vector<VkDescriptorPool> m_descriptorPools;
    uint32_t m_descriptorPoolSize = 30;
    uint32_t m_currentDescriptorPoolSize = 0;

    VkSampler m_shadowMapSampler = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_pointLightDescriptorSetLayout = VK_NULL_HANDLE;

    std::shared_ptr<Renderer> m_renderer;

    void createUniforms();

    void createDescriptorSet();

    void updateUniforms(uint32_t currentFrame,
                        glm::vec3 viewPosition);

    void updatePointLightUniforms(uint32_t currentFrame);

    void updatePointLightShadowMaps(uint32_t currentFrame) const;

    void updateSpotLightUniforms(uint32_t currentFrame);

    void updateSpotLightShadowMaps(uint32_t currentFrame) const;

    void createShadowMapSampler();

    void destroyShadowMapSampler();

    void renderPointLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                    const std::shared_ptr<PipelineManager>& pipelineManager,
                                    const std::vector<std::shared_ptr<RenderObject>>* objects,
                                    uint32_t currentFrame) const;

    void renderSpotLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   const std::shared_ptr<PipelineManager>& pipelineManager,
                                   const std::vector<std::shared_ptr<RenderObject>>* objects,
                                   uint32_t currentFrame) const;

    void createPointLightDescriptorSetLayout();

    void createCommandPool();

    void createDescriptorPool();

    [[nodiscard]] VkDescriptorPool getDescriptorPool();

    void updateLightMetadataUniform() const;
  };

} // namespace vke

#endif //VKE_LIGHTINGMANAGER_H
