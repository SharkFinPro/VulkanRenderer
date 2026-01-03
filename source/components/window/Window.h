#ifndef VKE_WINDOW_H
#define VKE_WINDOW_H

#include "../../utilities/EventSystem.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <unordered_map>

namespace vke {

  class VulkanEngine;

  struct ContentScaleEvent {
    float xscale;
    float yscale;
  };

  struct FramebufferResizeEvent {
    int width;
    int height;
  };

  struct KeyCallbackEvent {
    int key;
    int scancode;
    int action;
    int mods;
  };

  struct ScrollEvent {
    double xoffset;
    double yoffset;
  };

  class Window : public EventSystem<ContentScaleEvent, FramebufferResizeEvent, KeyCallbackEvent, ScrollEvent> {
  public:
    Window(int width,
           int height,
           const char* title,
           bool fullscreen,
           bool resizable);
    ~Window();

    [[nodiscard]] bool isOpen() const;

    void update();

    void getFramebufferSize(int* width,
                            int* height) const;

    [[nodiscard]] bool keyIsPressed(int key) const;

    [[nodiscard]] bool buttonIsPressed(int button) const;

    void getCursorPos(double& xpos,
                      double& ypos) const;

    void getPreviousCursorPos(double& xpos,
                              double& ypos) const;

    [[nodiscard]] double getScroll() const;

    [[nodiscard]] float getContentScale() const;

    static void scrollCallback(GLFWwindow* window,
                               double xoffset,
                               double yoffset);

    static void framebufferResizeCallback(GLFWwindow* window,
                                          int width,
                                          int height);

    static void contentScaleCallback(GLFWwindow* window,
                                     float xscale,
                                     float yscale);

    [[nodiscard]] GLFWwindow* getWindow() const;

  private:
    GLFWwindow* m_window;

    double m_previousMouseX;
    double m_previousMouseY;
    double m_mouseX = 0.0;
    double m_mouseY = 0.0;

    double m_scroll = 0.0;

    std::unordered_map<int, bool> m_keysPressed;

    float m_contentScale = 1.0f;

    static void keyCallback(GLFWwindow* window,
                            int key,
                            int scancode,
                            int action,
                            int mods);
  };

} // namespace vke

#endif //VKE_WINDOW_H
