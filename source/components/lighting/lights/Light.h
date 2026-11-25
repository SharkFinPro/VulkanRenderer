#ifndef VKE_LIGHT_H
#define VKE_LIGHT_H

#include "DirectionalShadow.h"
#include "LightRange.h"
#include "ShadowBias.h"
#include "ShadowFilter.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>

namespace vke {

  class ImageResource;
  class LogicalDevice;

  struct alignas(16) PointLightUniform {
    std::array<glm::mat4, 6> lightViewProjections;
    glm::vec3 position;
    float shadowNearPlane;
    glm::vec3 color;
    int32_t shadowMapIndex; // Into the point light shadow maps, or -1 when the light has none

    float ambient;
    float diffuse;
    float specular;
    float shadowRange;
    float range; // 0 when the light has no range

    alignas(16) glm::vec4 shadowBias; // The ShadowBias (constant, slope, normal offset), then the world size of a texel per unit of view depth
    alignas(16) glm::vec4 shadowFilter; // The filter radius in texels, then 0
  };

  struct alignas(16) SpotLightUniform {
    glm::mat4 lightViewProjection;
    glm::vec3 position;
    float ambient;
    glm::vec3 color;
    float diffuse;
    glm::vec3 direction;
    float specular;
    float coneAngle;
    float shadowRange;
    int32_t shadowMapIndex; // Into the spot light shadow maps, or -1 when the light has none
    float range; // 0 when the light has no range

    alignas(16) glm::vec4 shadowBias; // The ShadowBias (constant, slope, normal offset), then the world size of a texel per unit of view depth
    alignas(16) glm::vec4 shadowFilter; // The filter radius in texels, then 0
  };

  struct alignas(16) DirectionalLightUniform {
    std::array<glm::mat4, s_maxShadowCascades> cascadeViewProjections; // The fitted cascades; the rest are zero
    glm::vec4 cascadeTexelSizes; // World size of a texel, per cascade
    glm::vec3 direction; // The way the light travels, normalized
    float ambient;
    glm::vec3 color;
    float diffuse;
    float specular;
    int32_t shadowMapIndex; // Into the directional light shadow maps, or -1 when the light has none
    int32_t cascadeCount; // The fitted cascades, 0 when none
    float shadowDistance; // Fragments farther than this from the camera are lit; the last cascade ends here

    alignas(16) glm::vec4 shadowBias; // The ShadowBias (constant, slope, normal offset), then 0
    alignas(16) glm::vec4 shadowFilter; // The filter radius in texels, then 0
  };

  // All three are arrays in shader storage buffers, mirrored by the structs in shaders/common/Lighting.glsl
  static_assert(sizeof(PointLightUniform) == 480 && offsetof(PointLightUniform, shadowRange) == 428 &&
                offsetof(PointLightUniform, shadowMapIndex) == 412 && offsetof(PointLightUniform, range) == 432 &&
                offsetof(PointLightUniform, shadowBias) == 448 &&
                offsetof(PointLightUniform, shadowFilter) == 464);
  static_assert(sizeof(DirectionalLightUniform) == 352 && s_maxShadowCascades == 4 &&
                offsetof(DirectionalLightUniform, cascadeTexelSizes) == 256 &&
                offsetof(DirectionalLightUniform, direction) == 272 && offsetof(DirectionalLightUniform, ambient) == 284 &&
                offsetof(DirectionalLightUniform, color) == 288 && offsetof(DirectionalLightUniform, diffuse) == 300 &&
                offsetof(DirectionalLightUniform, specular) == 304 &&
                offsetof(DirectionalLightUniform, shadowMapIndex) == 308 &&
                offsetof(DirectionalLightUniform, cascadeCount) == 312 &&
                offsetof(DirectionalLightUniform, shadowDistance) == 316 &&
                offsetof(DirectionalLightUniform, shadowBias) == 320 &&
                offsetof(DirectionalLightUniform, shadowFilter) == 336);
  static_assert(sizeof(SpotLightUniform) == 160 && offsetof(SpotLightUniform, shadowRange) == 116 &&
                offsetof(SpotLightUniform, shadowMapIndex) == 120 &&
                offsetof(SpotLightUniform, range) == 124 &&
                offsetof(SpotLightUniform, shadowBias) == 128 &&
                offsetof(SpotLightUniform, shadowFilter) == 144);

  using LightUniform = std::variant<PointLightUniform, SpotLightUniform, DirectionalLightUniform>;

  enum class LightType {
    pointLight,
    spotLight,
    directionalLight
  };

  struct CommonLightData {
    glm::vec3 position;
    glm::vec3 color;
    float ambient;
    float diffuse;
    float specular;
  };

  class Light {
  public:
    // Format of every shadow map image; the shadow pipelines must declare the same depth
    // attachment format. D32Sfloat is also what makes ImageResource mark depth images sampled.
    static constexpr auto s_shadowMapFormat = vk::Format::eD32Sfloat;

    // Near plane of every shadow map projection; a shadow range must lie beyond it
    static constexpr float s_shadowNearPlane = 0.1f;

    static constexpr float s_defaultShadowRange = 1000.0f;

    Light(std::shared_ptr<LogicalDevice> logicalDevice,
          const CommonLightData& commonLightData);

    virtual ~Light();

    [[nodiscard]] glm::vec3 getPosition() const;
    [[nodiscard]] glm::vec3 getColor() const;
    [[nodiscard]] float getAmbient() const;
    [[nodiscard]] float getDiffuse() const;
    [[nodiscard]] float getSpecular() const;

    void setPosition(const glm::vec3& position);
    void setColor(const glm::vec3& color);
    void setAmbient(float ambient);
    void setDiffuse(float diffuse);
    void setSpecular(float specular);

    [[nodiscard]] vk::Extent2D getShadowMapExtent() const;

    // Distance the light reaches; 0 means unbounded and unattenuated. With a range the light falls off with inverse
    // square, windowed smoothly to zero at the range.
    [[nodiscard]] float getRange() const;

    // Throws std::invalid_argument unless range is 0 or finite and greater than s_shadowNearPlane
    void setRange(float range);

    // Distance from the light that its shadow map covers, as set. Fragments beyond it are lit, not shadowed.
    [[nodiscard]] float getShadowRange() const;

    // The shadow range clamped to the light's range, since shadows beyond where the light reaches would never be
    // seen; what the shadow projections, the caster culling and the shaders use
    [[nodiscard]] float getEffectiveShadowRange() const;

    // Throws std::invalid_argument unless range is finite and greater than s_shadowNearPlane
    void setShadowRange(float range);

    // How shadow lookups are moved off the surface (see ShadowBias), in texels of the light's shadow map. Throws
    // std::invalid_argument unless all three are finite and not negative.
    [[nodiscard]] ShadowBias getShadowBias() const;
    void setShadowBias(const ShadowBias& bias);

    // Radius of the percentage-closer filter on the light's shadow lookups, in texels of its shadow map (see
    // ShadowFilter.h); 0 takes a single sample. Throws std::invalid_argument unless finite and not negative.
    [[nodiscard]] float getShadowFilterRadius() const;
    void setShadowFilterRadius(float texels);

    [[nodiscard]] bool castsShadows() const;

    [[nodiscard]] virtual LightType getLightType() const = 0;

    [[nodiscard]] virtual LightUniform getUniform() const = 0;

    [[nodiscard]] std::shared_ptr<ImageResource> getShadowMapDepthImageResource() const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<ImageResource> m_shadowMapDepthImageResource;

    bool m_castsShadows = true;
    float m_shadowRange = s_defaultShadowRange;
    float m_range = 0.0f;
    ShadowBias m_shadowBias;
    float m_shadowFilterRadius = s_defaultShadowFilterRadius;
    vk::Extent2D m_shadowMapExtent = {
      1024,
      1024
    };

    glm::vec3 m_position;
    glm::vec3 m_color;
    float m_ambient;
    float m_diffuse;
    float m_specular;

    virtual void createShadowMap(vk::CommandPool commandPool) = 0;
  };

} // namespace vke

#endif //VKE_LIGHT_H
