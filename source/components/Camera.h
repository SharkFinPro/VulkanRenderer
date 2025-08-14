#ifndef VULKANPROJECT_CAMERA_H
#define VULKANPROJECT_CAMERA_H

#include "window/Window.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <chrono>
#include <memory>

namespace vke {

class Camera {
public:
  explicit Camera(glm::vec3 initialPosition = { 0, 0, 0 });

  [[nodiscard]] glm::mat4 getViewMatrix() const;

  [[nodiscard]] glm::vec3 getPosition() const;

  void setSpeed(float cameraSpeed);

  void processInput(const std::shared_ptr<Window>& window);

  void enable();

  void disable();

  bool isEnabled() const;

private:
  bool m_enabled;

  glm::vec3 m_position;
  glm::vec3 m_direction;

  struct SpeedSettings {
    float speed;
    float cameraSpeed;
    float scrollSpeed;
    float swivelSpeed;
  } m_speedSettings;

  struct Rotation {
    float pitch;
    float yaw;
  } m_rotation;

  std::chrono::time_point<std::chrono::steady_clock> m_previousTime;

  void handleMovement(const std::shared_ptr<Window>& window, float dt);
  void handleRotation(const std::shared_ptr<Window>& window);
  void handleZoom(const std::shared_ptr<Window>& window);
};

} // namespace vke

#endif //VULKANPROJECT_CAMERA_H