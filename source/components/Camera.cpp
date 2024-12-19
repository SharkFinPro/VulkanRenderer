#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <imgui.h>

constexpr auto UP = glm::vec3(0.0f, 1.0f, 0.0f);

Camera::Camera(const glm::vec3 pos)
  : position(pos), direction(0, 0, -1), speed(0), cameraSpeed(0), scrollSpeed(0), swivelSpeed(0), pitch(0),
    yaw(90), previousTime(std::chrono::steady_clock::now())
{}

glm::mat4 Camera::getViewMatrix() const
{
  return lookAt(position, position + direction, UP);
}

glm::vec3 Camera::getPosition() const
{
  return position;
}

void Camera::setSpeed(const float cameraSpeed)
{
  speed = cameraSpeed * 50.0f;

  this->cameraSpeed = speed * 0.005f;
  scrollSpeed = speed * 0.25f;
  swivelSpeed = speed * 0.005f;
}

void Camera::processInput(const std::shared_ptr<Window>& window)
{
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

  const auto pDirection = normalize(glm::vec3(
    -std::sin(glm::radians(yaw)),
    0.0f,
    std::cos(glm::radians(yaw))
  ));

  position += static_cast<float>(window->getScroll()) * scrollSpeed * direction;

  if (window->keyIsPressed(GLFW_KEY_W))
  {
    position += cameraSpeed * direction;
  }
  if (window->keyIsPressed(GLFW_KEY_S))
  {
    position -= cameraSpeed * direction;
  }

  if (window->keyIsPressed(GLFW_KEY_A))
  {
    position -= cameraSpeed * pDirection;
  }
  if (window->keyIsPressed(GLFW_KEY_D))
  {
    position += cameraSpeed * pDirection;
  }

  if (window->keyIsPressed(GLFW_KEY_SPACE))
  {
    position += cameraSpeed * UP;
  }
  if (window->keyIsPressed(GLFW_KEY_LEFT_SHIFT))
  {
    position -= cameraSpeed * UP;
  }
}