#ifndef VKE_LIGHTINGMANAGER_H
#define VKE_LIGHTINGMANAGER_H

#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace vke {

  class CommandBuffer;
  class DescriptorSet;
  struct DrawRun;
  class Light;
  class SpotLight;
  class LogicalDevice;
  class PipelineManager;
  class UniformBuffer;

  class LightingManager {
  public:
    explicit LightingManager(std::shared_ptr<LogicalDevice> logicalDevice);

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

    // The lights submitted this frame. Whether each casts shadows is up to the caller to ask (castsShadows).
    [[nodiscard]] const std::vector<std::shared_ptr<Light>>& getPointLightsToRender() const;

    [[nodiscard]] const std::vector<std::shared_ptr<Light>>& getSpotLightsToRender() const;

    // The light as a spot light if it is one that casts shadows, else null. The one filter both the shadow passes and
    // whoever builds their per-light draw lists use, so the i-th list always goes to the i-th such light.
    [[nodiscard]] static std::shared_ptr<SpotLight> asShadowCastingSpotLight(const std::shared_ptr<Light>& light);

    // Draws pointLightRuns into every shadow-casting point light's map, and into each shadow-casting spot light's map
    // its own entry of spotLightRuns, in the order of getSpotLightsToRender with the non-casting lights skipped (a light
    // without an entry draws nothing). The runs' first instances index the draw slots the scene descriptor set (set 0 of
    // the shadow pipelines) binds.
    void renderShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          std::span<const DrawRun> pointLightRuns,
                          std::span<const std::vector<DrawRun>> spotLightRuns,
                          vk::DescriptorSet sceneDescriptorSet,
                          uint32_t currentFrame) const;

    [[nodiscard]] vk::DescriptorSetLayout getPointLightDescriptorSetLayout() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

    std::shared_ptr<UniformBuffer> m_lightMetadataUniform;
    std::shared_ptr<UniformBuffer> m_spotLightsUniform;
    std::shared_ptr<UniformBuffer> m_pointLightsUniform;
    std::shared_ptr<UniformBuffer> m_cameraUniform;

    // Reused every frame to stage the point or spot light array before it is copied into its uniform buffer.
    std::vector<std::byte> m_lightUniformScratch;

    int m_prevNumPointLights = 0;
    int m_prevNumSpotLights = 0;

    std::vector<std::shared_ptr<Light>> m_pointLightsToRender;
    std::vector<std::shared_ptr<Light>> m_spotLightsToRender;

    vk::raii::CommandPool m_commandPool = nullptr;

    std::vector<vk::raii::DescriptorPool> m_descriptorPools;
    uint32_t m_descriptorPoolSize = 30;
    uint32_t m_currentDescriptorPoolSize = 0;

    vk::raii::Sampler m_shadowMapSampler = nullptr;

    vk::raii::DescriptorSetLayout m_pointLightDescriptorSetLayout = nullptr;

    void createUniforms();

    void createDescriptorSet();

    void updateUniforms(uint32_t currentFrame,
                        glm::vec3 viewPosition);

    void updatePointLightUniforms(uint32_t currentFrame);

    void updatePointLightShadowMaps(uint32_t currentFrame) const;

    void updateSpotLightUniforms(uint32_t currentFrame);

    void updateSpotLightShadowMaps(uint32_t currentFrame) const;

    void createShadowMapSampler();

    void renderPointLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                    const std::shared_ptr<PipelineManager>& pipelineManager,
                                    std::span<const DrawRun> runs,
                                    vk::DescriptorSet sceneDescriptorSet,
                                    uint32_t currentFrame) const;

    void renderSpotLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   const std::shared_ptr<PipelineManager>& pipelineManager,
                                   std::span<const std::vector<DrawRun>> spotLightRuns,
                                   vk::DescriptorSet sceneDescriptorSet,
                                   uint32_t currentFrame) const;

    void createPointLightDescriptorSetLayout();

    void createCommandPool();

    void createDescriptorPool();

    [[nodiscard]] vk::DescriptorPool getDescriptorPool();

    void updateLightMetadataUniform();

    static void beginShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                     const std::shared_ptr<Light>& light);

    static void endShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   const std::shared_ptr<Light>& light);
  };

} // namespace vke

#endif //VKE_LIGHTINGMANAGER_H
