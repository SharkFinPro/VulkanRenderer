#include "SmokeVolume.h"

namespace vke {

  SmokeVolume::SmokeVolume(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::CommandPool& commandPool,
                           glm::vec3 position)
    : ProceduralVolume(
        std::move(logicalDevice),
        commandPool,
        {
          -1.f, 0.f, -1.f,
          1.f, 1.f, 1.f
        },
        position,
        glm::vec3(1.5f, 15.0f, 1.5f))
  {}

  SmokeUniform SmokeVolume::getUniformData() const
  {
    return m_smokeUniform;
  }

  float SmokeVolume::getFrequency() const
  {
    return m_smokeUniform.frequency;
  }

  float SmokeVolume::getAmplitude() const
  {
    return m_smokeUniform.amplitude;
  }

  float SmokeVolume::getDensity() const
  {
    return m_smokeUniform.density;
  }

  float SmokeVolume::getYScale() const
  {
    return m_smokeUniform.yScale;
  }

  float SmokeVolume::getTime() const
  {
    return m_smokeUniform.time;
  }

  void SmokeVolume::setFrequency(const float frequency)
  {
    m_smokeUniform.frequency = frequency;
  }

  void SmokeVolume::setAmplitude(const float amplitude)
  {
    m_smokeUniform.amplitude = amplitude;
  }

  void SmokeVolume::setDensity(const float density)
  {
    m_smokeUniform.density = density;
  }

  void SmokeVolume::setYScale(const float yScale)
  {
    m_smokeUniform.yScale = yScale;
  }

  void SmokeVolume::setTime(const float time)
  {
    m_smokeUniform.time = time;
  }
} // vke