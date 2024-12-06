#ifndef VULKANPROJECT_WINDOW_H
#define VULKANPROJECT_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <unordered_map>
#include <GLFW/glfw3.h>

class Window {
public:
  Window(int width, int height, const char* title, VkInstance& instance, bool fullscreen);
  ~Window();

  [[nodiscard]] bool isOpen() const;

  void update();

  void getFramebufferSize(int* width, int* height) const;

  VkSurfaceKHR& getSurface();

  [[nodiscard]] bool keyIsPressed(int key) const;

  [[nodiscard]] bool buttonDown(int button) const;

  void getCursorPos(double& xpos, double& ypos) const;

  void getPreviousCursorPos(double& xpos, double& ypos) const;

  void initImGui() const;

  [[nodiscard]] double getScroll() const;

  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
  void createSurface();

  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
  GLFWwindow* window;

  VkInstance& instance;
  VkSurfaceKHR surface;

  double previousMouseX;
  double previousMouseY;
  double mouseX;
  double mouseY;

  double scroll;

  std::unordered_map<int, bool> keysPressed;
};


#endif //VULKANPROJECT_WINDOW_H
