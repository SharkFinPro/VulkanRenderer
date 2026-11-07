#ifndef VKE_RENDEROBJECT_H
#define VKE_RENDEROBJECT_H

#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <memory>

namespace vke {

  class LogicalDevice;
  class Material;
  class Model;
  class Texture;

  class RenderObject {
  public:
    RenderObject(const std::shared_ptr<LogicalDevice>& logicalDevice,
                 std::shared_ptr<Material> material,
                 std::shared_ptr<Model> model);

    ~RenderObject();

    void setPosition(glm::vec3 position);
    void setScale(glm::vec3 scale);
    void setScale(float scale);
    void setOrientationEuler(glm::vec3 orientation);
    void setOrientationQuat(glm::quat orientation);

    [[nodiscard]] glm::vec3 getPosition() const;
    [[nodiscard]] glm::vec3 getScale() const;
    [[nodiscard]] glm::vec3 getOrientationEuler() const;
    [[nodiscard]] glm::quat getOrientationQuat() const;

    [[nodiscard]] const std::shared_ptr<Model>& getModel() const;

    [[nodiscard]] const std::shared_ptr<Material>& getMaterial() const;

    [[nodiscard]] glm::mat4 getModelMatrix() const;

    [[nodiscard]] const std::shared_ptr<Texture>& getTexture() const;

    [[nodiscard]] const std::shared_ptr<Texture>& getSpecularMap() const;

    void setReflectivity(float reflectivity);

    [[nodiscard]] float getReflectivity() const;

    void setRefractivity(float refractivity);

    [[nodiscard]] float getRefractivity() const;

    void setIndexOfRefraction(float indexOfRefraction);

    [[nodiscard]] float getIndexOfRefraction() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<Material> m_material;
    std::shared_ptr<Model> m_model;

    glm::vec3 m_position = glm::vec3(0);
    glm::vec3 m_scale = glm::vec3(1);
    glm::quat m_orientation = glm::quat(1, 0, 0, 0);

    float m_reflectivity = 0.0f;
    float m_refractivity = 0.0f;
    float m_indexOfRefraction = 1.0f;
  };

} // namespace vke

#endif //VKE_RENDEROBJECT_H
