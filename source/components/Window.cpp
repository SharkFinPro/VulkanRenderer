#include "Window.h"
#include "../VulkanEngine.h"
#include "../core/instance/Instance.h"
#include <backends/imgui_impl_glfw.h>
#include <stdexcept>

Window::Window(const int width, const int height, const char* title, const std::shared_ptr<Instance>& instance,
               const bool fullscreen)
  : m_instance(instance), m_mouseX(0), m_mouseY(0), m_scroll(0)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  if (fullscreen)
  {
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    m_window = glfwCreateWindow(videoMode->width, videoMode->height, title, primaryMonitor, nullptr);
  }
  else
  {
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  }

  if (m_window == nullptr)
  {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwSetWindowUserPointer(m_window, this);
  glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

  glfwSetScrollCallback(m_window, scrollCallback);

  glfwGetCursorPos(m_window, &m_mouseX, &m_mouseY);
  m_previousMouseX = m_mouseX;
  m_previousMouseY = m_mouseY;

  m_surface = m_instance->createSurface(m_window);

  glfwSetKeyCallback(m_window, keyCallback);
}

Window::~Window()
{
  m_instance->destroySurface(m_surface);

  glfwDestroyWindow(m_window);
}

bool Window::isOpen() const
{
  return !glfwWindowShouldClose(m_window);
}

void Window::update()
{
  m_scroll = 0;

  glfwPollEvents();

  if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(m_window, true);
  }

  m_previousMouseX = m_mouseX;
  m_previousMouseY = m_mouseY;
  glfwGetCursorPos(m_window, &m_mouseX, &m_mouseY);
}

void Window::getFramebufferSize(int* width, int* height) const
{
  glfwGetFramebufferSize(m_window, width, height);
}

VkSurfaceKHR& Window::getSurface()
{
  return m_surface;
}

bool Window::keyIsPressed(const int key) const
{
  if (const auto keyNode = m_keysPressed.find(key); keyNode != m_keysPressed.end())
  {
    return keyNode->second;
  }

  return false;
}

bool Window::buttonDown(const int button) const
{
  return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}

void Window::getCursorPos(double& xpos, double& ypos) const
{
  xpos = m_mouseX;
  ypos = m_mouseY;
}

void Window::getPreviousCursorPos(double& xpos, double& ypos) const
{
  xpos = m_previousMouseX;
  ypos = m_previousMouseY;
}

void Window::initImGui() const
{
  ImGui_ImplGlfw_InitForVulkan(m_window, true);

  float xscale, yscale;
  glfwGetWindowContentScale(m_window, &xscale, &yscale);

  ImGui::GetStyle().ScaleAllSizes(xscale);
  ImGui::GetIO().FontGlobalScale = xscale;
}

double Window::getScroll() const
{
  return m_scroll;
}

void Window::scrollCallback(GLFWwindow* window, [[maybe_unused]] double xoffset, const double yoffset)
{
  const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
  app->m_scroll = yoffset;
}

void Window::framebufferResizeCallback(GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height)
{
  const auto app = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
  app->m_framebufferResized = true;
}

void Window::keyCallback(GLFWwindow* window, const int key, [[maybe_unused]] int scancode, const int action,
                         [[maybe_unused]] int mods)
{
  const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));

  app->m_keysPressed[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
}
