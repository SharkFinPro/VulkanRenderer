#include "Window.h"
#include <stdexcept>

Window::Window(int width, int height, GLFWframebuffersizefun framebufferResizeCallback)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

  if (window == nullptr)
  {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
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
