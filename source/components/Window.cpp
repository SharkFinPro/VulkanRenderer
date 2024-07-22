#include "Window.h"
#include <stdexcept>

#include "../VulkanEngine.h"

#include <backends/imgui_impl_glfw.h>

Window::Window(int width, int height, const char* title, GLFWframebuffersizefun framebufferResizeCallback, VkInstance& instance)
  : scroll(0), instance(instance)
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

  glfwSetScrollCallback(window, scrollCallback);

  glfwGetCursorPos(window, &mouseX, &mouseY);
  previousMouseX = mouseX;
  previousMouseY = mouseY;

  createSurface();
}

Window::~Window()
{
  vkDestroySurfaceKHR(instance, surface, nullptr);

  glfwDestroyWindow(window);
}

bool Window::isOpen()
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

void Window::getFramebufferSize(int* width, int* height)
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

bool Window::keyDown(int key) const
{
  return glfwGetKey(window, key) == GLFW_PRESS;
}

bool Window::buttonDown(int button) const
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

void Window::scrollCallback(GLFWwindow* window, [[maybe_unused]] double xoffset, double yoffset)
{
  auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  app->scroll = yoffset;
}
