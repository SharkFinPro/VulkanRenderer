#include "Cloud.h"

namespace vke {

  Cloud::Cloud(std::shared_ptr<LogicalDevice> logicalDevice,
               const vk::CommandPool& commandPool)
    : ProceduralVolume(
        logicalDevice,
        commandPool,
        {
          -1.f, -1.f, -1.f,
          1.f, 1.f, 1.f
        },
        glm::vec3(0.f, 500.f, 0.f),
        glm::vec3(5500.f, 400.f, 5500.f)
      )
  {}

  CloudUniform Cloud::getUniformData() const
  {
    return m_uniformData;
  }

  float Cloud::getFrequency() const
  {
    return m_uniformData.frequency;
  }

  float Cloud::getAmplitude() const
  {
    return m_uniformData.amplitude;
  }

  float Cloud::getDensity() const
  {
    return m_uniformData.density;
  }

  float Cloud::getYScale() const
  {
    return m_uniformData.yScale;
  }

  float Cloud::getTime() const
  {
    return m_uniformData.time;
  }

  void Cloud::setFrequency(const float frequency)
  {
    m_uniformData.frequency = frequency;
  }

  void Cloud::setAmplitude(const float amplitude)
  {
    m_uniformData.amplitude = amplitude;
  }

  void Cloud::setDensity(const float density)
  {
    m_uniformData.density = density;
  }

  void Cloud::setYScale(const float yScale)
  {
    m_uniformData.yScale = yScale;
  }

  void Cloud::setTime(const float time)
  {
    m_uniformData.time = time;
  }

} // vke