#ifndef VULKANPROJECT_VULKANAPP_H
#define VULKANPROJECT_VULKANAPP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

class VulkanApp {
public:
  void run();

private:
  void initVulkan();
  void mainLoop();
  void cleanup();
  void initWindow();

private:
  GLFWwindow* window;
};


#endif //VULKANPROJECT_VULKANAPP_H
