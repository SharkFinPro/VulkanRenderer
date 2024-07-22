#ifndef VULKANPROJECT_WINDOW_H
#define VULKANPROJECT_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
public:
  Window(int width, int height, const char* title, GLFWframebuffersizefun framebufferResizeCallback, VkInstance& instance);
  ~Window();

  [[nodiscard]] bool isOpen() const;

  void update();

  void getFramebufferSize(int* width, int* height);

  VkSurfaceKHR& getSurface();

  [[nodiscard]] bool keyDown(int key) const;

  [[nodiscard]] bool buttonDown(int button) const;

  void getCursorPos(double& xpos, double& ypos) const;

  void getPreviousCursorPos(double& xpos, double& ypos) const;

  void initImGui() const;

  [[nodiscard]] double getScroll() const;

  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
  void createSurface();

private:
  GLFWwindow* window;

  VkInstance& instance;
  VkSurfaceKHR surface;

  double previousMouseX;
  double previousMouseY;
  double mouseX;
  double mouseY;

  double scroll;
};


#endif //VULKANPROJECT_WINDOW_H
