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

private:
  GLFWwindow* window;
};


#endif //VULKANPROJECT_WINDOW_H
