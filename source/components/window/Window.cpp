#include "Window.h"
#include <stdexcept>
#include <vector>

namespace vke {

  Window::Window(const EngineConfig::Window& config)
    : m_closeOnEscape(config.closeOnEscape),
      m_windowedRect{ .width = static_cast<int>(config.width), .height = static_cast<int>(config.height) }
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    if (config.fullscreen)
    {
      glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);

      GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

      const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

      m_window = glfwCreateWindow(videoMode->width, videoMode->height, config.title.c_str(), primaryMonitor, nullptr);
    }
    else
    {
      m_window = glfwCreateWindow(
        static_cast<int>(config.width),
        static_cast<int>(config.height),
        config.title.c_str(),
        nullptr,
        nullptr
      );
    }

    if (m_window == nullptr)
    {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }

    m_hasWindowedPosition = !config.fullscreen;

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

    glfwSetDropCallback(m_window, dropCallback);

    glfwSetWindowCloseCallback(m_window, closeCallback);
  }

  Window::~Window()
  {
    glfwDestroyWindow(m_window);
  }

  bool Window::isOpen() const
  {
    return !glfwWindowShouldClose(m_window);
  }

  void Window::requestClose()
  {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);

    emitCloseRequest();
  }

  void Window::cancelClose()
  {
    glfwSetWindowShouldClose(m_window, GLFW_FALSE);
  }

  void Window::close()
  {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
  }

  void Window::emitCloseRequest()
  {
    // A listener that routes its own closes through requestClose() would otherwise recurse without end.
    if (m_emittingCloseRequest)
    {
      return;
    }

    m_emittingCloseRequest = true;
    emit(CloseRequestEvent{});
    m_emittingCloseRequest = false;
  }

  void Window::update()
  {
    m_scroll = 0;

    glfwPollEvents();

    m_previousMouseX = m_mouseX;
    m_previousMouseY = m_mouseY;
    glfwGetCursorPos(m_window, &m_mouseX, &m_mouseY);
  }

  void Window::setFullscreen(const bool fullscreen)
  {
    if (fullscreen == isFullscreen())
    {
      return;
    }

    if (fullscreen)
    {
      enterFullscreen();
    }
    else
    {
      leaveFullscreen();
    }

    if (isFullscreen() == fullscreen)
    {
      emit(FullscreenChangeEvent{fullscreen});
    }
  }

  bool Window::isFullscreen() const
  {
    return glfwGetWindowMonitor(m_window) != nullptr;
  }

  void Window::enterFullscreen()
  {
    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

    if (monitorCount == 0)
    {
      return;
    }

    // A maximized or minimized window reports its maximized or zero size, not the placement to come back to.
    m_windowedMaximized = glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED) == GLFW_TRUE;
    if (m_windowedMaximized || glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE)
    {
      glfwRestoreWindow(m_window);
    }

    glfwGetWindowPos(m_window, &m_windowedRect.x, &m_windowedRect.y);
    glfwGetWindowSize(m_window, &m_windowedRect.width, &m_windowedRect.height);
    m_hasWindowedPosition = true;

    std::vector<WindowRect> monitorRects(monitorCount);
    for (int i = 0; i < monitorCount; ++i)
    {
      const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
      glfwGetMonitorPos(monitors[i], &monitorRects[i].x, &monitorRects[i].y);
      monitorRects[i].width = mode->width;
      monitorRects[i].height = mode->height;
    }

    const auto index = findMostOverlappedArea(m_windowedRect, monitorRects);
    GLFWmonitor* monitor = index.has_value() ? monitors[*index] : glfwGetPrimaryMonitor();

    // Keeping the monitor's current mode avoids a mode switch, so this is borderless fullscreen where the platform
    // offers it. Like a window created fullscreen, it stays on screen when focus moves to another window.
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowAttrib(m_window, GLFW_AUTO_ICONIFY, GLFW_FALSE);
    glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
  }

  void Window::leaveFullscreen()
  {
    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

    std::vector<WindowRect> workAreas(monitorCount);
    for (int i = 0; i < monitorCount; ++i)
    {
      glfwGetMonitorWorkarea(monitors[i], &workAreas[i].x, &workAreas[i].y, &workAreas[i].width, &workAreas[i].height);
    }

    WindowRect fallback;
    glfwGetMonitorWorkarea(glfwGetWindowMonitor(m_window), &fallback.x, &fallback.y, &fallback.width, &fallback.height);

    const WindowRect rect = m_hasWindowedPosition
      ? restoreWindowedRect(m_windowedRect, workAreas, fallback)
      : centerInArea(m_windowedRect.width, m_windowedRect.height, fallback);

    glfwSetWindowMonitor(m_window, nullptr, rect.x, rect.y, rect.width, rect.height, GLFW_DONT_CARE);

    if (m_windowedMaximized)
    {
      glfwMaximizeWindow(m_window);
    }
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
    if (glfwGetMouseButton(m_window, button) == GLFW_PRESS)
    {
      return true;
    }

    for (GLFWwindow* inputWindow : m_inputWindows)
    {
      if (glfwGetMouseButton(inputWindow, button) == GLFW_PRESS)
      {
        return true;
      }
    }

    return false;
  }

  void Window::handleKey(const int key,
                         const int scancode,
                         const int action,
                         const int mods)
  {
    // A press or release that arrives through any path settles the key's state, so any secondary window it was
    // recorded against must not release it again later; handleSecondaryWindowKey() re-records its own presses.
    if (action != GLFW_REPEAT)
    {
      m_keyOwnerWindow.erase(key);
    }

    m_keysPressed[key] = action == GLFW_PRESS || action == GLFW_REPEAT;

    emit(KeyCallbackEvent{key, scancode, action, mods});

    if (m_closeOnEscape && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
      requestClose();
    }
  }

  void Window::handleScroll(const double xoffset,
                            const double yoffset)
  {
    // One poll can deliver several scroll events; update() resets the total once per frame.
    m_scroll += yoffset;

    emit(ScrollEvent{xoffset, yoffset});
  }

  void Window::handleSecondaryWindowKey(GLFWwindow* sourceWindow,
                                        const int key,
                                        const int scancode,
                                        const int action,
                                        const int mods)
  {
    handleKey(key, scancode, action, mods);

    if (key >= 0 && action == GLFW_PRESS)
    {
      m_keyOwnerWindow[key] = sourceWindow;
    }
  }

  void Window::addInputWindow(GLFWwindow* window)
  {
    m_inputWindows.push_back(window);
  }

  void Window::removeInputWindow(GLFWwindow* window)
  {
    std::erase(m_inputWindows, window);

    // GLFW will not deliver release events for a window it is about to destroy, so any key still recorded as held
    // through it must be released here first, the same way ImGui_ImplGlfw_DestroyWindow releases its own key state.
    for (auto it = m_keyOwnerWindow.begin(); it != m_keyOwnerWindow.end();)
    {
      if (it->second == window)
      {
        const int key = it->first;
        it = m_keyOwnerWindow.erase(it);
        handleKey(key, 0, GLFW_RELEASE, 0);
      }
      else
      {
        ++it;
      }
    }
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
    app->handleScroll(xoffset, yoffset);
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
    app->handleKey(key, scancode, action, mods);
  }

  void Window::dropCallback(GLFWwindow* window,
                            const int pathCount,
                            const char* paths[])
  {
    const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));

    std::vector<std::string> pathsVec(pathCount);
    for (int i = 0; i < pathCount; ++i)
    {
      pathsVec[i] = paths[i];
    }

    app->emit(DropEvent{ pathsVec });
  }

  void Window::closeCallback(GLFWwindow* window)
  {
    const auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));

    // GLFW has already set the close flag, so listeners see the same state as requestClose().
    app->emitCloseRequest();
  }
} // namespace vke