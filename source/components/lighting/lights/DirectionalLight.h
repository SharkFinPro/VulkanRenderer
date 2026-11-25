#ifndef VKE_DIRECTIONALLIGHT_H
#define VKE_DIRECTIONALLIGHT_H

#include "DirectionalShadow.h"
#include "Light.h"

#include <array>

namespace vke {

  // A sun: it has a direction and no position or range (the inherited position, range and shadow range are unused).
  // Its shadow is cascaded: the camera frustum out to the shadow distance is split into up to s_maxShadowCascades slices,
  // each fitted by an orthographic map (see fitShadowCascade) in a layer of one 2D array image. updateCascades fits them
  // each frame; fragments beyond the shadow distance are lit.
  class DirectionalLight final : public Light {
  public:
    // Side of the shadow map in texels
    static constexpr uint32_t s_shadowMapSize = 2048;

    DirectionalLight(std::shared_ptr<LogicalDevice> logicalDevice,
                     const CommonLightData& commonLightData,
                     glm::vec3 direction,
                     vk::CommandPool commandPool);

    // The way the light travels, normalized
    [[nodiscard]] glm::vec3 getDirection() const;

    // Throws std::invalid_argument for a zero-length or non-finite direction
    void setDirection(const glm::vec3& direction);

    // The number of cascades to fit, 1 to s_maxShadowCascades (default 4). Throws std::invalid_argument outside that.
    [[nodiscard]] uint32_t getCascadeCount() const;
    void setCascadeCount(uint32_t count);

    // How far from the camera the shadows reach (default 100); the last cascade ends there. Throws
    // std::invalid_argument unless it is finite and greater than 0.
    [[nodiscard]] float getShadowDistance() const;
    void setShadowDistance(float distance);

    // Blends the logarithmic (1) and uniform (0) distribution of the splits (default 0.75). Throws
    // std::invalid_argument unless it is finite and in [0, 1].
    [[nodiscard]] float getCascadeSplitLambda() const;
    void setCascadeSplitLambda(float lambda);

    // Fits the cascades to the camera for this frame. The shadow distance is cut to farPlane; casterBounds is the world
    // box of every shadow caster (may be empty). With nothing between the near plane and that distance no cascade is fitted.
    void updateCascades(const CascadeCamera& camera,
                        float farPlane,
                        const AABB& casterBounds);

    // The cascades the last updateCascades fitted; 0 before the first call
    [[nodiscard]] uint32_t getFittedCascadeCount() const;

    // Throws std::out_of_range unless cascade < s_maxShadowCascades
    [[nodiscard]] const glm::mat4& getCascadeViewProjection(uint32_t cascade) const;

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

  private:
    glm::vec3 m_direction = glm::vec3(0, -1, 0);
    uint32_t m_cascadeCount = s_maxShadowCascades;
    float m_shadowDistance = 100.0f;
    float m_cascadeSplitLambda = 0.75f;

    uint32_t m_fittedCascadeCount = 0;
    float m_fittedShadowDistance = 0.0f;
    std::array<glm::mat4, s_maxShadowCascades> m_cascadeViewProjections{};
    std::array<float, s_maxShadowCascades> m_cascadeTexelSizes{};

    void createShadowMap(vk::CommandPool commandPool) override;
  };

} // namespace vke

#endif //VKE_DIRECTIONALLIGHT_H
