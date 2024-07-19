#include "Window.h"
#include <stdexcept>

Window::Window(int width, int height, const char* title, GLFWframebuffersizefun framebufferResizeCallback)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);

  if (window == nullptr)
  {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

  glfwGetCursorPos(window, &mouseX, &mouseY);
  previousMouseX = mouseX;
  previousMouseY = mouseY;
}

Window::~Window()
{
  glfwDestroyWindow(window);
}

bool Window::isOpen()
{
  return !glfwWindowShouldClose(window);
}

void Window::update()
{
  glfwPollEvents();

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }

  previousMouseX = mouseX;
  previousMouseY = mouseY;
  glfwGetCursorPos(window, &mouseX, &mouseY);
}

void Window::getFramebufferSize(int* width, int* height)
{
  glfwGetFramebufferSize(window, width, height);
}

void Window::createSurface(VkInstance instance, VkSurfaceKHR* surface)
{
  if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create window surface!");
  }
}

bool Window::keyDown(int key) const
{
  return glfwGetKey(window, key) == GLFW_PRESS;
}

bool Window::buttonDown(int button) const
{
  return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

void Window::getCursorPos(double &xpos, double &ypos) const
{
  xpos = mouseX;
  ypos = mouseY;
}

void Window::getPreviousCursorPos(double &xpos, double &ypos) const
{
  xpos = previousMouseX;
  ypos = previousMouseY;
}

int Window::getWidth() const
{
  int width;
  glfwGetWindowSize(window, &width, nullptr);
  return width;
}

int Window::getHeight() const
{
  int height;
  glfwGetWindowSize(window, nullptr, &height);
  return height;
}

GLFWwindow* Window::getWindow() const
{
  return window;
}
