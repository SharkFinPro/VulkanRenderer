#ifndef VKE_LIGHT_H
#define VKE_LIGHT_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>
#include <array>
#include <memory>
#include <variant>

namespace vke {

  class LogicalDevice;
  class RenderTarget;

  struct alignas(16) PointLightUniform {
    std::array<glm::mat4, 6> lightViewProjections;
    glm::vec3 position;
    float padding1;
    glm::vec3 color;
    float padding2;

    float ambient;
    float diffuse;
    float specular;
    float padding3;
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
  };

  using LightUniform = std::variant<PointLightUniform, SpotLightUniform>;

  enum class LightType {
    pointLight,
    spotLight
  };

class Light {
public:
  Light(const std::shared_ptr<LogicalDevice>& logicalDevice,
        const glm::vec3& position,
        const glm::vec3& color,
        float ambient,
        float diffuse,
        float specular);

  virtual ~Light() = default;

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

  [[nodiscard]] VkImage getShadowMap() const;

  [[nodiscard]] VkImageView getShadowMapView() const;

  [[nodiscard]] uint32_t getShadowMapSize() const;

  [[nodiscard]] bool castsShadows() const;

  [[nodiscard]] virtual LightType getLightType() const = 0;

  [[nodiscard]] virtual LightUniform getUniform() const = 0;

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::shared_ptr<RenderTarget> m_shadowMapRenderTarget;

  bool m_castsShadows = true;
  uint32_t m_shadowMapSize = 1024;

  glm::vec3 m_position;
  glm::vec3 m_color;
  float m_ambient;
  float m_diffuse;
  float m_specular;

  virtual void createShadowMap(const VkCommandPool& commandPool) = 0;
};

} // namespace vke

#endif //VKE_LIGHT_H
