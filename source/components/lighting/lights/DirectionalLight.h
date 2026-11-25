#ifndef VKE_DIRECTIONALLIGHT_H
#define VKE_DIRECTIONALLIGHT_H

#include "Light.h"

namespace vke {

  // A sun: it has a direction and no position or range (the inherited position, range and shadow range are unused).
  // Its shadow is one orthographic map fitted to a box (see directionalShadowViewProjection): casters outside the box are
  // not in the map, and fragments outside it are lit.
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

    // The center and half the side of the box the shadow map covers
    [[nodiscard]] glm::vec3 getShadowCenter() const;
    [[nodiscard]] float getShadowHalfExtent() const;

    // Throws std::invalid_argument for a non-finite center
    void setShadowCenter(const glm::vec3& center);

    // Throws std::invalid_argument unless the half extent is finite and greater than 0
    void setShadowHalfExtent(float halfExtent);

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

    [[nodiscard]] glm::mat4 getLightViewProjectionMatrix() const;

  private:
    glm::vec3 m_direction = glm::vec3(0, -1, 0);
    glm::vec3 m_shadowCenter = glm::vec3(0, 0, 0);
    float m_shadowHalfExtent = 50.0f;

    void createShadowMap(vk::CommandPool commandPool) override;
  };

} // namespace vke

#endif //VKE_DIRECTIONALLIGHT_H
