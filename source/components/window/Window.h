#ifndef VKE_WINDOW_H
#define VKE_WINDOW_H

#include "../../EngineConfig.h"
#include "../../utilities/EventSystem.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace vke {

  struct WindowConfig;

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

  struct DropEvent {
    std::vector<std::string> paths;
  };

  struct CloseRequestEvent {};

  class Window : public EventSystem<ContentScaleEvent, FramebufferResizeEvent, KeyCallbackEvent, ScrollEvent, DropEvent,
                                    CloseRequestEvent> {
  public:
    explicit Window(const EngineConfig::Window& config);
    ~Window();

    [[nodiscard]] bool isOpen() const;

    // Marks the window to close and emits CloseRequestEvent. A listener may call cancelClose() to veto; the decision
    // must be made inside the listener, because the frame loop checks isOpen() before any GUI code runs again.
    void requestClose();

    void cancelClose();

    // Closes without emitting CloseRequestEvent, for when the application has already confirmed.
    void close();

    void update();

    void getFramebufferSize(int* width,
                            int* height) const;

    [[nodiscard]] bool keyIsPressed(int key) const;

    [[nodiscard]] bool buttonIsPressed(int button) const;

    // The state update and KeyCallbackEvent emission shared by the main window's own keyCallback and
    // handleSecondaryWindowKey() below.
    void handleKey(int key,
                   int scancode,
                   int action,
                   int mods);

    // Same as handleKey(), for scroll events from a detached window.
    void handleScroll(double xoffset,
                      double yoffset);

    // Entry point for a key event from a GLFW window other than the main one (a detached ImGui viewport). Records
    // which window a pressed key came from, so removeInputWindow() can synthesize its release: GLFW never delivers
    // one for a window it is about to destroy, so without this a key held when its window closes would stay "down"
    // forever (e.g. a stuck WASD key after redocking the scene view).
    void handleSecondaryWindowKey(GLFWwindow* sourceWindow,
                                  int key,
                                  int scancode,
                                  int action,
                                  int mods);

    // Registers a GLFW window (a detached ImGui viewport) whose mouse buttons buttonIsPressed() should also check.
    // GLFW tracks per-window button state regardless of callbacks, so no event needs forwarding for it.
    void addInputWindow(GLFWwindow* window);

    // Unregisters the window, releasing (through handleKey(), so KeyCallbackEvent still fires) every key still
    // recorded as held through it.
    void removeInputWindow(GLFWwindow* window);

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

    std::vector<GLFWwindow*> m_inputWindows;

    // Which registered input window last pressed a given key, so removeInputWindow() knows what to release.
    std::unordered_map<int, GLFWwindow*> m_keyOwnerWindow;

    float m_contentScale = 1.0f;

    const bool m_closeOnEscape;

    bool m_emittingCloseRequest = false;

    void emitCloseRequest();

    static void keyCallback(GLFWwindow* window,
                            int key,
                            int scancode,
                            int action,
                            int mods);

    static void dropCallback(GLFWwindow* window,
                             int pathCount,
                             const char* paths[]);

    static void closeCallback(GLFWwindow* window);
  };

} // namespace vke

#endif //VKE_WINDOW_H
