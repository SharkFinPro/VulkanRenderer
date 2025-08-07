#ifndef VULKANPROJECT_WINDOW_H
#define VULKANPROJECT_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <unordered_map>

class Instance;

class Window {
public:
  Window(int width, int height, const char* title, const std::shared_ptr<Instance>& instance, bool fullscreen);
  ~Window();

  [[nodiscard]] bool isOpen() const;

  void update();

  void getFramebufferSize(int* width, int* height) const;

  [[nodiscard]] VkSurfaceKHR& getSurface();

  [[nodiscard]] bool keyIsPressed(int key) const;

  [[nodiscard]] bool buttonIsPressed(int button) const;

  void getCursorPos(double& xpos, double& ypos) const;

  void getPreviousCursorPos(double& xpos, double& ypos) const;

  void initImGui() const;

  [[nodiscard]] double getScroll() const;

  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
  GLFWwindow* m_window;

  std::shared_ptr<Instance> m_instance;

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;

  double m_previousMouseX;
  double m_previousMouseY;
  double m_mouseX;
  double m_mouseY;

  double m_scroll;

  std::unordered_map<int, bool> m_keysPressed;

  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};


#endif //VULKANPROJECT_WINDOW_H
