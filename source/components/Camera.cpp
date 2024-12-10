#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

constexpr auto UP = glm::vec3(0.0f, 1.0f, 0.0f);

Camera::Camera(const glm::vec3 pos)
  : position(pos), speed(0), cameraSpeed(0), scrollSpeed(0), swivelSpeed(0), previousTime(std::chrono::steady_clock::now())
{}

glm::mat4 Camera::getViewMatrix() const
{
  return glm::lookAt(position, position + direction, UP);
}

glm::vec3 Camera::getPosition() const
{
  return position;
}

void Camera::setSpeed(const float cameraSpeed)
{
  speed = cameraSpeed * 50.0f;

  this->cameraSpeed = speed;
  scrollSpeed = speed * 0.25f;
  swivelSpeed = speed * 0.005f;
}

void Camera::processInput(const std::shared_ptr<Window>& window)
{
  const auto currentTime = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(currentTime - previousTime).count();
  previousTime = currentTime;

  if (window->buttonDown(GLFW_MOUSE_BUTTON_RIGHT))
  {
    double mx, my, omx, omy;
    window->getCursorPos(mx, my);
    window->getPreviousCursorPos(omx, omy);

    const auto deltaMX = static_cast<float>(mx - omx) * swivelSpeed;
    const auto deltaMY = static_cast<float>(my - omy) * swivelSpeed;

    yaw += deltaMX;
    pitch -= deltaMY;

    pitch = std::clamp(pitch, -89.9f, 89.9f);
  }

  direction = glm::normalize(glm::vec3(
    std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
    std::sin(glm::radians(pitch)),
    std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))
  ));

  pDirection = glm::normalize(glm::vec3(
    pDirection.x = -std::sin(glm::radians(yaw)),
    0.0f,
    pDirection.z = std::cos(glm::radians(yaw))
  ));

  position += static_cast<float>(window->getScroll()) * scrollSpeed * direction;

  if (window->keyIsPressed(GLFW_KEY_W))
  {
    position += cameraSpeed * direction * dt;
  }
  if (window->keyIsPressed(GLFW_KEY_S))
  {
    position -= cameraSpeed * direction * dt;
  }

  if (window->keyIsPressed(GLFW_KEY_A))
  {
    position -= cameraSpeed * pDirection * dt;
  }
  if (window->keyIsPressed(GLFW_KEY_D))
  {
    position += cameraSpeed * pDirection * dt;
  }

  if (window->keyIsPressed(GLFW_KEY_SPACE))
  {
    position += cameraSpeed * UP * dt;
  }
  if (window->keyIsPressed(GLFW_KEY_LEFT_SHIFT))
  {
    position -= cameraSpeed * UP * dt;
  }
}