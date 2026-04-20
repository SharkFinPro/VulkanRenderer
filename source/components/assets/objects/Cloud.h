#ifndef VULKANPROJECT_CLOUD_H
#define VULKANPROJECT_CLOUD_H

#include "ProceduralVolume.h"
#include <memory>

namespace vke {

  class LogicalDevice;

  struct CloudUniform {
    float frequency = 20.0f;
    float amplitude = 3.0f;
    float density = 0.15f;
    float yScale = 0.15f;
    float time = 0.0f;
  };

  class Cloud : public ProceduralVolume {
  public:
    Cloud(std::shared_ptr<LogicalDevice> logicalDevice,
          const vk::CommandPool& commandPool);

    [[nodiscard]] CloudUniform getUniformData() const;

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
    CloudUniform m_uniformData;
  };
} // vke

#endif //VULKANPROJECT_CLOUD_H