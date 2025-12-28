#include "Window.h"
#include <stdexcept>

namespace vke {

  Window::Window(const int width,
                 const int height,
                 const char* title,
                 std::shared_ptr<Instance> instance,
                 const bool fullscreen)
    : m_instance(std::move(instance))
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

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

    float xscale;
    glfwGetWindowContentScale(m_window, &xscale, nullptr);
    m_contentScale = xscale;

    glfwSetKeyCallback(m_window, keyCallback);

    glfwSetWindowContentScaleCallback(m_window, contentScaleCallback);
  }

  Window::~Window()
  {
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

  void Window::getFramebufferSize(int* width,
                                  int* height) const
  {
    glfwGetFramebufferSize(m_window, width, height);
  }

  bool Window::keyIsPressed(const int key) const
  {
    if (const auto keyNode = m_keysPressed.find(key); keyNode != m_keysPressed.end())
    {
      return keyNode->second;
    }

    return false;
  }

  bool Window::buttonIsPressed(const int button) const
  {
    return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
  }

  void Window::getCursorPos(double& xpos,
                            double& ypos) const
  {
    xpos = m_mouseX;
    ypos = m_mouseY;
  }

  void Window::getPreviousCursorPos(double& xpos,
                                    double& ypos) const
  {
    xpos = m_previousMouseX;
    ypos = m_previousMouseY;
  }

  double Window::getScroll() const
  {
    return m_scroll;
  }

  float Window::getContentScale() const
  {
    return m_contentScale;
  }

  void Window::scrollCallback(GLFWwindow* window,
                              const double xoffset,
                              const double yoffset)
  {
    const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
    app->m_scroll = yoffset;

    app->emit(ScrollEvent{xoffset, yoffset});
  }

  void Window::framebufferResizeCallback(GLFWwindow* window,
                                         const int width,
                                         const int height)
  {
    const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
    app->emit(FramebufferResizeEvent{width, height});
  }

  void Window::contentScaleCallback(GLFWwindow* window,
                                    const float xscale,
                                    const float yscale)
  {
    const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));

    app->m_contentScale = xscale;

    app->emit(ContentScaleEvent{xscale, yscale});
  }

  GLFWwindow* Window::getWindow() const
  {
    return m_window;
  }

  void Window::keyCallback(GLFWwindow* window,
                           const int key,
                           const int scancode,
                           const int action,
                           const int mods)
  {
    const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));

    app->m_keysPressed[key] = action == GLFW_PRESS || action == GLFW_REPEAT;

    app->emit(KeyCallbackEvent{key, scancode, action, mods});
  }

} // namespace vke