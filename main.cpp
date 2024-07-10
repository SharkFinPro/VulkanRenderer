#include <iostream>
#include "source/VulkanApp.h"

int main()
{
  try
  {
    VulkanApp app;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}