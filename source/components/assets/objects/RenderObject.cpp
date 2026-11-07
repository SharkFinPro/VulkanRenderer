#include "RenderObject.h"
#include "Material.h"
#include "Model.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include <glm/gtc/matrix_transform.hpp>

namespace vke {

  RenderObject::RenderObject(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             std::shared_ptr<Material> material,
                             std::shared_ptr<Model> model)
    : m_logicalDevice(logicalDevice),
      m_material(std::move(material)),
      m_model(std::move(model))
  {
  }

  RenderObject::~RenderObject()
  {
    // Frames in flight may still draw this object's model and sample its material.
    m_logicalDevice->retire(std::move(m_material));
    m_logicalDevice->retire(std::move(m_model));
  }

  void RenderObject::draw(const std::shared_ptr<CommandBuffer>& commandBuffer,
                          const uint32_t slot) const
  {
    m_model->draw(commandBuffer, slot);
  }

  void RenderObject::setPosition(const glm::vec3 position)
  {
    m_position = position;
  }

  void RenderObject::setScale(const glm::vec3 scale)
  {
    m_scale = scale;
  }

  void RenderObject::setScale(const float scale)
  {
    m_scale = glm::vec3(scale);
  }

  void RenderObject::setOrientationEuler(const glm::vec3 orientation)
  {
    m_orientation = glm::quat(glm::radians(orientation));
  }

  void RenderObject::setOrientationQuat(const glm::quat orientation)
  {
    m_orientation = glm::normalize(orientation);
  }

  glm::vec3 RenderObject::getPosition() const
  {
    return m_position;
  }

  glm::vec3 RenderObject::getScale() const
  {
    return m_scale;
  }

  glm::vec3 RenderObject::getOrientationEuler() const
  {
    return glm::degrees(glm::eulerAngles(m_orientation));
  }

  glm::quat RenderObject::getOrientationQuat() const
  {
    return m_orientation;
  }

  const std::shared_ptr<Model>& RenderObject::getModel() const
  {
    return m_model;
  }

  const std::shared_ptr<Material>& RenderObject::getMaterial() const
  {
    return m_material;
  }

  glm::mat4 RenderObject::getModelMatrix() const
  {
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), m_position)
                            * glm::mat4(m_orientation)
                            * glm::scale(glm::mat4(1.0f), m_scale);

    return model;
  }

  const std::shared_ptr<Texture>& RenderObject::getTexture() const
  {
    return m_material->getTexture();
  }

  const std::shared_ptr<Texture>& RenderObject::getSpecularMap() const
  {
    return m_material->getSpecularMap();
  }

  void RenderObject::setReflectivity(const float reflectivity)
  {
    m_reflectivity = reflectivity;
  }

  float RenderObject::getReflectivity() const
  {
    return m_reflectivity;
  }

  void RenderObject::setRefractivity(const float refractivity)
  {
    m_refractivity = refractivity;
  }

  float RenderObject::getRefractivity() const
  {
    return m_refractivity;
  }

  void RenderObject::setIndexOfRefraction(const float indexOfRefraction)
  {
    m_indexOfRefraction = indexOfRefraction;
  }

  float RenderObject::getIndexOfRefraction() const
  {
    return m_indexOfRefraction;
  }

} // namespace vke