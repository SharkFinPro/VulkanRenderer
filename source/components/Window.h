#ifndef VULKANPROJECT_WINDOW_H
#define VULKANPROJECT_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
public:
  Window(int width, int height, const char* title, GLFWframebuffersizefun framebufferResizeCallback);
  ~Window();

  bool isOpen();

  void update();

  void getFramebufferSize(int* width, int* height);

  void createSurface(VkInstance instance, VkSurfaceKHR* surface);

  [[nodiscard]] bool keyDown(int key) const;

  [[nodiscard]] bool buttonDown(int button) const;

  void getCursorPos(double& xpos, double& ypos) const;

  void getPreviousCursorPos(double& xpos, double& ypos) const;

  void initImGui() const;

  [[nodiscard]] double getScroll() const;

  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
  GLFWwindow* window;

  double previousMouseX;
  double previousMouseY;
  double mouseX;
  double mouseY;

  double scroll;
};


#endif //VULKANPROJECT_WINDOW_H
