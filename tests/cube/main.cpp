#include <iostream>
#include <source/VulkanEngine.h>

int main()
{
  try
  {
    VulkanEngine app;

    auto texture = app.loadTexture("assets/textures/container.png");
    auto model = app.loadModel("assets/models/cube.obj");

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