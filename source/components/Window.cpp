#include "Window.h"
#include "../VulkanEngine.h"
#include <stdexcept>
#include <backends/imgui_impl_glfw.h>

Window::Window(const int width, const int height, const char* title, const std::shared_ptr<Instance>& instance,
               const bool fullscreen)
  : instance(instance), scroll(0)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  if (fullscreen)
  {
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);

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
  vkDestroySurfaceKHR(instance->getInstance(), surface, nullptr);

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
  if (glfwCreateWindowSurface(instance->getInstance(), window, nullptr, &surface) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create window surface!");
  }
}

VkSurfaceKHR& Window::getSurface()
{
  return surface;
}

bool Window::keyIsPressed(const int key) const
{
  if (const auto keyNode = keysPressed.find(key); keyNode != keysPressed.end())
  {
    return keyNode->second;
  }

  return false;
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

  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);

  ImGui::GetStyle().ScaleAllSizes(xscale);
  ImGui::GetIO().FontGlobalScale = xscale;
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

void Window::keyCallback(GLFWwindow* window, const int key, [[maybe_unused]] int scancode, const int action,
                         [[maybe_unused]] int mods)
{
  const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));

  app->keysPressed[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
}
