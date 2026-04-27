#ifndef VULKANPROJECT_SMOKEVOLUME_H
#define VULKANPROJECT_SMOKEVOLUME_H

#include "ProceduralVolume.h"

namespace vke {

  struct SmokeUniform {
    float frequency = 8.0f;
    float amplitude = 2.0f;
    float density = 0.15f;
    float yScale = 3.0f;
    float time = 0.0f;
  };

  class SmokeVolume : public ProceduralVolume {
  public:
    SmokeVolume(std::shared_ptr<LogicalDevice> logicalDevice,
                const vk::CommandPool& commandPool,
                glm::vec3 position);

    [[nodiscard]] SmokeUniform getUniformData() const;

    [[nodiscard]] float getFrequency() const;
    [[nodiscard]] float getAmplitude() const;
    [[nodiscard]] float getDensity() const;
    [[nodiscard]] float getYScale() const;
    [[nodiscard]] float getTime() const;

    void setFrequency(float frequency);
    void setAmplitude(float amplitude);
    void setDensity(float density);
    void setYScale(float yScale);
    void setTime(float time);

  private:
    SmokeUniform m_smokeUniform;
  };

} // vke

#endif //VULKANPROJECT_SMOKEVOLUME_H
