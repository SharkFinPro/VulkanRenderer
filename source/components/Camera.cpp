#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

constexpr auto UP = glm::vec3(0.0f, 1.0f, 0.0f);

Camera::Camera(const glm::vec3 initialPosition)
  : m_position(initialPosition), m_direction(0, 0, -1),
    m_speedSettings {
      .speed = 0,
      .cameraSpeed = 0,
      .scrollSpeed = 0,
      .swivelSpeed = 0
    },
    m_rotation {
      .pitch = 0,
      .yaw = 90
    },
    m_previousTime(std::chrono::steady_clock::now())
{}

glm::mat4 Camera::getViewMatrix() const
{
  return lookAt(m_position, m_position + m_direction, UP);
}

glm::vec3 Camera::getPosition() const
{
  return m_position;
}

void Camera::setSpeed(const float cameraSpeed)
{
  m_speedSettings.speed = cameraSpeed * 50.0f;

  m_speedSettings.cameraSpeed = m_speedSettings.speed * 0.0005f;
  m_speedSettings.scrollSpeed = m_speedSettings.speed * 0.25f;
  m_speedSettings.swivelSpeed = m_speedSettings.speed * 0.005f;
}

void Camera::processInput(const std::shared_ptr<Window>& window)
{
  const auto currentTime = std::chrono::steady_clock::now();
  m_dt = std::chrono::duration<float>(currentTime - m_previousTime).count() * 1000.0f;
  m_previousTime = currentTime;

  handleRotation(window);

  handleZoom(window);

  handleMovement(window);
}

void Camera::handleMovement(const std::shared_ptr<Window>& window)
{
  const auto pDirection = normalize(glm::vec3(
    -std::sin(glm::radians(m_rotation.yaw)),
    0.0f,
    std::cos(glm::radians(m_rotation.yaw))
  ));

  if (window->keyIsPressed(GLFW_KEY_W))
  {
    m_position += m_speedSettings.cameraSpeed * m_direction * m_dt;
  }
  if (window->keyIsPressed(GLFW_KEY_S))
  {
    m_position -= m_speedSettings.cameraSpeed * m_direction * m_dt;
  }

  if (window->keyIsPressed(GLFW_KEY_A))
  {
    m_position -= m_speedSettings.cameraSpeed * pDirection * m_dt;
  }
  if (window->keyIsPressed(GLFW_KEY_D))
  {
    m_position += m_speedSettings.cameraSpeed * pDirection * m_dt;
  }

  if (window->keyIsPressed(GLFW_KEY_SPACE))
  {
    m_position += m_speedSettings.cameraSpeed * UP * m_dt;
  }
  if (window->keyIsPressed(GLFW_KEY_LEFT_SHIFT))
  {
    m_position -= m_speedSettings.cameraSpeed * UP * m_dt;
  }
}

void Camera::handleRotation(const std::shared_ptr<Window>& window)
{
  if (window->buttonIsPressed(GLFW_MOUSE_BUTTON_RIGHT))
  {
    double mx, my, omx, omy;
    window->getCursorPos(mx, my);
    window->getPreviousCursorPos(omx, omy);

    const auto deltaMX = static_cast<float>(mx - omx) * m_speedSettings.swivelSpeed;
    const auto deltaMY = static_cast<float>(my - omy) * m_speedSettings.swivelSpeed;

    m_rotation.yaw += deltaMX;
    m_rotation.pitch -= deltaMY;

    m_rotation.pitch = std::clamp(m_rotation.pitch, -89.9f, 89.9f);
  }

  m_direction = normalize(glm::vec3(
    std::cos(glm::radians(m_rotation.yaw)) * std::cos(glm::radians(m_rotation.pitch)),
    std::sin(glm::radians(m_rotation.pitch)),
    std::sin(glm::radians(m_rotation.yaw)) * std::cos(glm::radians(m_rotation.pitch))
  ));
}

void Camera::handleZoom(const std::shared_ptr<Window>& window)
{
  m_position += static_cast<float>(window->getScroll()) * m_speedSettings.scrollSpeed * m_direction;
}
