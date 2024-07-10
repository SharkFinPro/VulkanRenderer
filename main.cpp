#include <iostream>
#include "source/VulkanEngine.h"

int main()
{
  try
  {
    VulkanEngine app;

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