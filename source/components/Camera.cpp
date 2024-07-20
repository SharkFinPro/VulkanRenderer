#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

const glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f);

Camera::Camera(glm::vec3 pos)
  : position(pos), speed(0), cameraSpeed(0), scrollSpeed(0), swivelSpeed(0)
{}

glm::mat4 Camera::getViewMatrix() const
{
  return glm::lookAt(position, position + direction, UP);
}

glm::vec3 Camera::getPosition() const
{
  return position;
}

void Camera::setSpeed(float cameraSpeed_)
{
  speed = cameraSpeed_;

  cameraSpeed = speed * 0.025f;
  scrollSpeed = speed * 15.0f;
  swivelSpeed = speed / 5.0f;
}

void Camera::processInput(const std::shared_ptr<Window>& window)
{
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
    position += cameraSpeed * UP;
  }
  if (window->keyDown(GLFW_KEY_LEFT_SHIFT))
  {
    position -= cameraSpeed * UP;
  }
}