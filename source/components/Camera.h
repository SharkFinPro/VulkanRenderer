#ifndef VULKANPROJECT_CAMERA_H
#define VULKANPROJECT_CAMERA_H

#include "Window.h"
#include <glm/glm.hpp>
#include <memory>
#include <chrono>

class Camera {
public:
  explicit Camera(glm::vec3 pos = { 0, 0, 0 });

  [[nodiscard]] glm::mat4 getViewMatrix() const;

  [[nodiscard]] glm::vec3 getPosition() const;

  void setSpeed(float cameraSpeed);

  void processInput(const std::shared_ptr<Window>& window);

private:
  glm::vec3 position;
  glm::vec3 direction;

  struct SpeedSettings {
    float speed;
    float cameraSpeed;
    float scrollSpeed;
    float swivelSpeed;
  } speedSettings;

  struct Rotation {
    float pitch;
    float yaw;
  } rotation;

  std::chrono::time_point<std::chrono::steady_clock> previousTime;
  float dt = 0;

  void handleMovement(const std::shared_ptr<Window>& window);
  void handleRotation(const std::shared_ptr<Window>& window);
  void handleZoom(const std::shared_ptr<Window>& window);
};

#endif //VULKANPROJECT_CAMERA_H