#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(glm::vec3 pos)
  : position(pos)
{}

glm::mat4 Camera::getViewMatrix() const
{
  return glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::getPosition() const
{
  return position;
}

void Camera::setSpeed(float cameraSpeed)
{
  speed = cameraSpeed;
}

void Camera::processInput(const std::shared_ptr<Window>& window)
{
  float cameraSpeed = speed * 0.025f;
  float scrollSpeed = speed * 15.0f;
  float swivelSpeed = speed / 5.0f;

  if (window->buttonDown(GLFW_MOUSE_BUTTON_RIGHT))
  {
    double mx, my, omx, omy;
    window->getCursorPos(mx, my);
    window->getPreviousCursorPos(omx, omy);

    auto deltaMX = static_cast<float>(mx - omx) * swivelSpeed;
    auto deltaMY = static_cast<float>(my - omy) * swivelSpeed;

    yaw += deltaMX;
    pitch -= deltaMY;
  }

  pitch = std::clamp(pitch, -89.9f, 89.9f);

  direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
  direction.y = std::sin(glm::radians(pitch));
  direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

  pDirection.x = -std::sin(glm::radians(yaw));
  pDirection.z = std::cos(glm::radians(yaw));

  position += static_cast<float>(window->getScroll()) * scrollSpeed * direction;

  if (window->keyDown(GLFW_KEY_W))
  {
    position += cameraSpeed * direction;
  }
  if (window->keyDown(GLFW_KEY_S))
  {
    position -= cameraSpeed * direction;
  }

  if (window->keyDown(GLFW_KEY_A))
  {
    position -= cameraSpeed * pDirection;
  }
  if (window->keyDown(GLFW_KEY_D))
  {
    position += cameraSpeed * pDirection;
  }

  if (window->keyDown(GLFW_KEY_SPACE))
  {
    position += cameraSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
  }
  if (window->keyDown(GLFW_KEY_LEFT_SHIFT))
  {
    position -= cameraSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
  }
}