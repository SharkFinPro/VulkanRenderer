#include <iostream>
#include <source/VulkanEngine.h>

int main()
{
  try
  {
    auto vulkanEngineOptions = VulkanEngineOptions{};
    vulkanEngineOptions.WINDOW_WIDTH = 800;
    vulkanEngineOptions.WINDOW_HEIGHT = 600;
    vulkanEngineOptions.WINDOW_TITLE = "Vulkan";
    vulkanEngineOptions.VERTEX_SHADER_FILE = "assets/shaders/vert.spv";
    vulkanEngineOptions.FRAGMENT_SHADER_FILE = "assets/shaders/frag.spv";

    VulkanEngine app(&vulkanEngineOptions);

    auto texture = app.loadTexture("assets/textures/viking_room.png");
    auto model = app.loadModel("assets/models/viking_room.obj");

    auto object = app.loadRenderObject(texture, model);

    while (app.isActive())
    {
      app.render();
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}