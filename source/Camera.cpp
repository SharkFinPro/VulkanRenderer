#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

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

void Camera::processInput(const std::shared_ptr<Window> &window)
{
  float cameraSpeed = speed * window->getDeltaTime() * 0.05f;

  if (window->buttonDown(GLFW_MOUSE_BUTTON_RIGHT))
  {
    double mx, my, omx, omy;
    window->getCursorPos(mx, my);
    window->getPreviousCursorPos(omx, omy);

    auto deltaMX = static_cast<float>(mx - omx) / (static_cast<float>(window->getWidth()) / 800.0f);
    auto deltaMY = static_cast<float>(my - omy) / (static_cast<float>(window->getHeight()) / 600.0f);

    yaw += deltaMX;
    pitch -= deltaMY;
  }

  if (pitch > 89.0f)
  {
    pitch = 89.0f;
  }
  else if (pitch < -89.0f)
  {
    pitch = -89.0f;
  }

  direction.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
  direction.y = std::sin(glm::radians(pitch));
  direction.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

  pDirection.x = -std::sin(glm::radians(yaw));
  pDirection.z = std::cos(glm::radians(yaw));

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