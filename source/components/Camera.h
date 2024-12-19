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

  float speed;
  float cameraSpeed;
  float scrollSpeed;
  float swivelSpeed;

  float pitch;
  float yaw;

  std::chrono::time_point<std::chrono::steady_clock> previousTime;
};

#endif //VULKANPROJECT_CAMERA_H