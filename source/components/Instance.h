#ifndef VULKANPROJECT_INSTANCE_H
#define VULKANPROJECT_INSTANCE_H

#include <vulkan/vulkan.h>
#include <vector>

const std::vector<const char*> validationLayers = {
  "VK_LAYER_KHRONOS_validation"
};

class Instance {
public:
  Instance();
  ~Instance();

  VkInstance& getInstance();

private:
  bool checkValidationLayerSupport();

  std::vector<const char*> getRequiredExtensions();

private:
  VkInstance instance;
};


#endif //VULKANPROJECT_INSTANCE_H
