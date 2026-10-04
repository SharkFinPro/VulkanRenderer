#ifndef VKE_GPUCAPABILITIES_H
#define VKE_GPUCAPABILITIES_H

#include <vulkan/vulkan_raii.hpp>
#include <cstdint>
#include <string>

namespace vke {

  // What the selected GPU offers, for applications to adapt their UI or content to.
  struct GpuCapabilities {
    std::string deviceName;
    vk::PhysicalDeviceType deviceType = vk::PhysicalDeviceType::eOther;
    uint32_t apiVersion = 0;
    // Ray tracing extensions and every feature the engine enables for them are available.
    bool rayTracing = false;
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    vk::Format depthFormat = vk::Format::eUndefined;
  };

} // namespace vke

#endif //VKE_GPUCAPABILITIES_H
