#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <imgui.h>

constexpr auto UP = glm::vec3(0.0f, 1.0f, 0.0f);

Camera::Camera(const glm::vec3 pos)
  : position(pos), direction(0, 0, -1),
    speedSettings {
      .speed = 0,
      .cameraSpeed = 0,
      .scrollSpeed = 0,
      .swivelSpeed = 0
    },
    rotation {
      .pitch = 0,
      .yaw = 90
    },
    previousTime(std::chrono::steady_clock::now())
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
  speedSettings.speed = cameraSpeed * 50.0f;

  speedSettings.cameraSpeed = speedSettings.speed * 0.005f;
  speedSettings.scrollSpeed = speedSettings.speed * 0.25f;
  speedSettings.swivelSpeed = speedSettings.speed * 0.005f;
}

void Camera::processInput(const std::shared_ptr<Window>& window)
{
  handleRotation(window);

  handleZoom(window);

  handleMovement(window);
}

void Camera::handleMovement(const std::shared_ptr<Window>& window)
{
  const auto pDirection = normalize(glm::vec3(
    -std::sin(glm::radians(rotation.yaw)),
    0.0f,
    std::cos(glm::radians(rotation.yaw))
  ));

  if (window->keyIsPressed(GLFW_KEY_W))
  {
    position += speedSettings.cameraSpeed * direction;
  }
  if (window->keyIsPressed(GLFW_KEY_S))
  {
    position -= speedSettings.cameraSpeed * direction;
  }

  if (window->keyIsPressed(GLFW_KEY_A))
  {
    position -= speedSettings.cameraSpeed * pDirection;
  }
  if (window->keyIsPressed(GLFW_KEY_D))
  {
    position += speedSettings.cameraSpeed * pDirection;
  }

  if (window->keyIsPressed(GLFW_KEY_SPACE))
  {
    position += speedSettings.cameraSpeed * UP;
  }
  if (window->keyIsPressed(GLFW_KEY_LEFT_SHIFT))
  {
    position -= speedSettings.cameraSpeed * UP;
  }
}

void Camera::handleRotation(const std::shared_ptr<Window>& window)
{
  if (window->buttonDown(GLFW_MOUSE_BUTTON_RIGHT))
  {
    double mx, my, omx, omy;
    window->getCursorPos(mx, my);
    window->getPreviousCursorPos(omx, omy);

    const auto deltaMX = static_cast<float>(mx - omx) * speedSettings.swivelSpeed;
    const auto deltaMY = static_cast<float>(my - omy) * speedSettings.swivelSpeed;

    rotation.yaw += deltaMX;
    rotation.pitch -= deltaMY;

    rotation.pitch = std::clamp(rotation.pitch, -89.9f, 89.9f);
  }

  direction = glm::normalize(glm::vec3(
    std::cos(glm::radians(rotation.yaw)) * std::cos(glm::radians(rotation.pitch)),
    std::sin(glm::radians(rotation.pitch)),
    std::sin(glm::radians(rotation.yaw)) * std::cos(glm::radians(rotation.pitch))
  ));
}

void Camera::handleZoom(const std::shared_ptr<Window>& window)
{
  position += static_cast<float>(window->getScroll()) * speedSettings.scrollSpeed * direction;
}
