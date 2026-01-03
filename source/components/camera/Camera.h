#ifndef VKE_CAMERA_H
#define VKE_CAMERA_H

#include "../../EngineConfig.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <chrono>
#include <memory>

namespace vke {

  class Window;

  class Camera {
  public:
    explicit Camera(const EngineConfig::Camera& config);

    [[nodiscard]] glm::mat4 getViewMatrix() const;

    [[nodiscard]] glm::vec3 getPosition() const;

    void setSpeed(float cameraSpeed);

    void processInput(const std::shared_ptr<Window>& window);

    void enable();

    void disable();

    [[nodiscard]] bool isEnabled() const;

  private:
    bool m_enabled = true;

    glm::vec3 m_position;
    glm::vec3 m_direction = glm::vec3(0, 0, -1);

    struct SpeedSettings {
      float speed = 0;
      float cameraSpeed = 0;
      float scrollSpeed = 0;
      float swivelSpeed = 0;
    } m_speedSettings;

    struct Rotation {
      float pitch = 0;
      float yaw = 90;
    } m_rotation;

    std::chrono::time_point<std::chrono::steady_clock> m_previousTime;

    void handleMovement(const std::shared_ptr<Window>& window, float dt);
    void handleRotation(const std::shared_ptr<Window>& window);
    void handleZoom(const std::shared_ptr<Window>& window);
  };

} // namespace vke

#endif //VKE_CAMERA_H
