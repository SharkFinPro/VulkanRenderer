#ifndef VKE_LIGHT_H
#define VKE_LIGHT_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <cstddef>
#include <memory>
#include <variant>

namespace vke {

  class ImageResource;
  class LogicalDevice;

  struct alignas(16) PointLightUniform {
    std::array<glm::mat4, 6> lightViewProjections;
    glm::vec3 position;
    float padding1;
    glm::vec3 color;
    float padding2;

    float ambient;
    float diffuse;
    float specular;
    float shadowRange;
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
  };

  // Both are arrays in shader storage buffers, mirrored by the structs in shaders/common/Lighting.glsl
  static_assert(sizeof(PointLightUniform) == 432 && offsetof(PointLightUniform, shadowRange) == 428);
  static_assert(sizeof(SpotLightUniform) == 128 && offsetof(SpotLightUniform, shadowRange) == 116);

  using LightUniform = std::variant<PointLightUniform, SpotLightUniform>;

  enum class LightType {
    pointLight,
    spotLight
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

    static constexpr float s_defaultShadowRange = 100.0f;

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

    // Distance from the light that its shadow map covers. Fragments beyond it are lit, not shadowed;
    // the light itself reaches any distance.
    [[nodiscard]] float getShadowRange() const;

    // Throws std::invalid_argument unless range is greater than s_shadowNearPlane
    void setShadowRange(float range);

    [[nodiscard]] bool castsShadows() const;

    [[nodiscard]] virtual LightType getLightType() const = 0;

    [[nodiscard]] virtual LightUniform getUniform() const = 0;

    [[nodiscard]] std::shared_ptr<ImageResource> getShadowMapDepthImageResource() const;

  protected:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<ImageResource> m_shadowMapDepthImageResource;

    bool m_castsShadows = true;
    float m_shadowRange = s_defaultShadowRange;
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
