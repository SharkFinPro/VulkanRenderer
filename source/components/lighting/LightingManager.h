#ifndef VKE_LIGHTINGMANAGER_H
#define VKE_LIGHTINGMANAGER_H

#include "../renderingManager/renderer3D/DrawRun.h"
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace vke {

  class CommandBuffer;
  class DescriptorSet;
  class Light;
  class PointLight;
  class SpotLight;
  class DirectionalLight;
  class LogicalDevice;
  class PipelineManager;
  class UniformBuffer;

  // What the shadow passes draw: runs of render objects and runs of volume surface chunks, six lists per shadow-casting
  // point light (one per cube face, in face order), one per shadow-casting spot light and s_maxShadowCascades per
  // shadow-casting directional light (one per cascade, in cascade order; see renderShadowMaps).
  struct ShadowDrawLists {
    std::span<const std::vector<DrawRun>> pointLightFaceRuns;
    std::span<const std::vector<DrawRun>> spotLightRuns;
    std::span<const std::vector<DrawRun>> volumePointLightFaceRuns;
    std::span<const std::vector<DrawRun>> volumeSpotLightRuns;
    std::span<const std::vector<DrawRun>> directionalLightRuns;
    std::span<const std::vector<DrawRun>> volumeDirectionalLightRuns;
  };

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

    // At most 4 directional lights are taken per frame; later ones are ignored.
    [[nodiscard]] std::shared_ptr<Light> createDirectionalLight(glm::vec3 direction,
                                                                glm::vec3 color,
                                                                float ambient,
                                                                float diffuse,
                                                                float specular = 1.0f);

    void renderLight(const std::shared_ptr<Light>& light);

    [[nodiscard]] std::shared_ptr<DescriptorSet> getLightingDescriptorSet() const;

    void clearLightsToRender();

    void update(uint32_t currentFrame, glm::vec3 viewPosition);

    // Writes the directional lights' storage buffer for the frame and gives each shadow-casting one its shadow map index.
    // The uniforms carry the fitted cascades, so this must run after every directional light's updateCascades (done by
    // Renderer3D::updateSceneData) and before the frame is submitted; update() does not write it.
    void writeDirectionalLights(uint32_t currentFrame);

    // The lights submitted this frame. Whether each casts shadows is up to the caller to ask (castsShadows).
    [[nodiscard]] const std::vector<std::shared_ptr<Light>>& getPointLightsToRender() const;

    [[nodiscard]] const std::vector<std::shared_ptr<Light>>& getSpotLightsToRender() const;

    [[nodiscard]] const std::vector<std::shared_ptr<Light>>& getDirectionalLightsToRender() const;

    // The light as a spot light if it is one that casts shadows, else null. The one filter both the shadow passes and
    // whoever builds their per-light draw lists use, so the i-th list always goes to the i-th such light.
    [[nodiscard]] static std::shared_ptr<SpotLight> asShadowCastingSpotLight(const std::shared_ptr<Light>& light);

    // The same for point lights, whose lists come six to a light
    [[nodiscard]] static std::shared_ptr<PointLight> asShadowCastingPointLight(const std::shared_ptr<Light>& light);

    // The same for directional lights
    [[nodiscard]] static std::shared_ptr<DirectionalLight> asShadowCastingDirectionalLight(const std::shared_ptr<Light>& light);

    // Draws into each face of every shadow-casting point light's cube map its own entry of pointLightFaceRuns (six per
    // light, in the order of getPointLightsToRender with the non-casting lights skipped, then in face order), and into each
    // shadow-casting spot light's map its own entry of spotLightRuns, in the order of getSpotLightsToRender likewise, and
    // into each fitted cascade of each shadow-casting directional light's map its own entry of directionalLightRuns
    // (s_maxShadowCascades per light, so cascade c of the n-th light is entry n * s_maxShadowCascades + c), with the
    // lights in the order of getDirectionalLightsToRender (a light, face or cascade without an entry draws nothing). The
    // volume lists are drawn the same way after them, with the volume surface
    // pipelines. The runs' first instances index the draw slots the scene descriptor set (set 0 of the shadow pipelines)
    // binds.
    void renderShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          const ShadowDrawLists& lists,
                          vk::DescriptorSet sceneDescriptorSet,
                          uint32_t currentFrame) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

    std::shared_ptr<UniformBuffer> m_lightMetadataUniform;
    std::shared_ptr<UniformBuffer> m_spotLightsUniform;
    std::shared_ptr<UniformBuffer> m_pointLightsUniform;
    std::shared_ptr<UniformBuffer> m_directionalLightsUniform;
    std::shared_ptr<UniformBuffer> m_cameraUniform;

    // Reused every frame to stage the point or spot light array before it is copied into its uniform buffer.
    std::vector<std::byte> m_lightUniformScratch;

    int m_prevNumPointLights = 0;
    int m_prevNumSpotLights = 0;
    int m_prevNumDirectionalLights = 0;

    std::vector<std::shared_ptr<Light>> m_pointLightsToRender;
    std::vector<std::shared_ptr<Light>> m_spotLightsToRender;
    std::vector<std::shared_ptr<Light>> m_directionalLightsToRender;

    vk::raii::CommandPool m_commandPool = nullptr;

    std::vector<vk::raii::DescriptorPool> m_descriptorPools;
    uint32_t m_descriptorPoolSize = 30;
    uint32_t m_currentDescriptorPoolSize = 0;

    vk::raii::Sampler m_shadowMapSampler = nullptr;

    // The point lights' maps hold reversed depth, so they compare the other way
    vk::raii::Sampler m_pointShadowMapSampler = nullptr;

    void createUniforms();

    void createDescriptorSet();

    void updateUniforms(uint32_t currentFrame,
                        glm::vec3 viewPosition);

    void updatePointLightUniforms(uint32_t currentFrame);

    void updatePointLightShadowMaps(uint32_t currentFrame) const;

    void updateSpotLightUniforms(uint32_t currentFrame);

    void updateSpotLightShadowMaps(uint32_t currentFrame) const;

    void updateDirectionalLightShadowMaps(uint32_t currentFrame) const;

    void createShadowMapSampler();

    void renderPointLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                    const std::shared_ptr<PipelineManager>& pipelineManager,
                                    std::span<const std::vector<DrawRun>> faceRuns,
                                    std::span<const std::vector<DrawRun>> volumeFaceRuns,
                                    vk::DescriptorSet sceneDescriptorSet) const;

    void renderSpotLightShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   const std::shared_ptr<PipelineManager>& pipelineManager,
                                   std::span<const std::vector<DrawRun>> spotLightRuns,
                                   std::span<const std::vector<DrawRun>> volumeSpotLightRuns,
                                   vk::DescriptorSet sceneDescriptorSet,
                                   uint32_t currentFrame) const;

    void renderDirectionalShadowMaps(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                     const std::shared_ptr<PipelineManager>& pipelineManager,
                                     std::span<const std::vector<DrawRun>> directionalLightRuns,
                                     std::span<const std::vector<DrawRun>> volumeDirectionalLightRuns,
                                     vk::DescriptorSet sceneDescriptorSet) const;

    void createCommandPool();

    void createDescriptorPool();

    [[nodiscard]] vk::DescriptorPool getDescriptorPool();

    void updateLightMetadataUniform();

    static void transitionShadowMapForRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                                const std::shared_ptr<Light>& light);

    // Begins rendering into the depth view, cleared to clearDepth, and sets the viewport and scissor to its extent
    static void beginShadowRendering(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                     vk::ImageView depthView,
                                     vk::Extent2D extent,
                                     float clearDepth);

    static void transitionShadowMapForSampling(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                               const std::shared_ptr<Light>& light);
  };

} // namespace vke

#endif //VKE_LIGHTINGMANAGER_H
