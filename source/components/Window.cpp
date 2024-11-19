#include "Window.h"
#include "../VulkanEngine.h"
#include <stdexcept>
#include <backends/imgui_impl_glfw.h>

Window::Window(const int width, const int height, const char* title, VkInstance& instance, const bool fullscreen)
  : instance(instance), scroll(0)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  if (fullscreen)
  {
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    window = glfwCreateWindow(videoMode->width, videoMode->height, title, primaryMonitor, nullptr);
  }
  else
  {
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  }

  if (window == nullptr)
  {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

  glfwSetScrollCallback(window, scrollCallback);

  glfwGetCursorPos(window, &mouseX, &mouseY);
  previousMouseX = mouseX;
  previousMouseY = mouseY;

  createSurface();

  glfwSetKeyCallback(window, keyCallback);
}

Window::~Window()
{
  vkDestroySurfaceKHR(instance, surface, nullptr);

  glfwDestroyWindow(window);
}

bool Window::isOpen() const
{
  return !glfwWindowShouldClose(window);
}

void Window::update()
{
  scroll = 0;

  glfwPollEvents();

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }

  previousMouseX = mouseX;
  previousMouseY = mouseY;
  glfwGetCursorPos(window, &mouseX, &mouseY);
}

void Window::getFramebufferSize(int* width, int* height) const
{
  glfwGetFramebufferSize(window, width, height);
}

void Window::createSurface()
{
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create window surface!");
  }
}

VkSurfaceKHR& Window::getSurface()
{
  return surface;
}

bool Window::keyDown(const int key) const
{
  return glfwGetKey(window, key) == GLFW_PRESS;
}

bool Window::inputActive(const int key) const
{
  switch (key)
  {
    case GLFW_KEY_W:
      return keysPressed.forward;
    case GLFW_KEY_S:
      return keysPressed.backward;
    case GLFW_KEY_A:
      return keysPressed.left;
    case GLFW_KEY_D:
      return keysPressed.right;
    case GLFW_KEY_SPACE:
      return keysPressed.up;
    case GLFW_KEY_LEFT_SHIFT:
      return keysPressed.down;
    default:
      return false;
  }
}

bool Window::buttonDown(const int button) const
{
  return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

void Window::getCursorPos(double& xpos, double& ypos) const
{
  xpos = mouseX;
  ypos = mouseY;
}

void Window::getPreviousCursorPos(double& xpos, double& ypos) const
{
  xpos = previousMouseX;
  ypos = previousMouseY;
}

void Window::initImGui() const
{
  ImGui_ImplGlfw_InitForVulkan(window, true);
}

double Window::getScroll() const
{
  return scroll;
}

void Window::scrollCallback(GLFWwindow* window, [[maybe_unused]] double xoffset, const double yoffset)
{
  const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
  app->scroll = yoffset;
}

void Window::framebufferResizeCallback(GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height)
{
  const auto app = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));

  const bool pressed = action == GLFW_PRESS || action == GLFW_REPEAT;

  switch (key)
  {
    case GLFW_KEY_W:
      app->keysPressed.forward = pressed;
      break;
    case GLFW_KEY_S:
      app->keysPressed.backward = pressed;
      break;
    case GLFW_KEY_A:
      app->keysPressed.left = pressed;
      break;
    case GLFW_KEY_D:
      app->keysPressed.right = pressed;
      break;
    case GLFW_KEY_SPACE:
      app->keysPressed.up = pressed;
      break;
    case GLFW_KEY_LEFT_SHIFT:
      app->keysPressed.down = pressed;
      break;
    default: ;
  }
}
