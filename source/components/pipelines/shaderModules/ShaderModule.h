#ifndef VKE_SHADERMODULE_H
#define VKE_SHADERMODULE_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  class ShaderModule {
  public:
    ShaderModule(const std::shared_ptr<LogicalDevice>& logicalDevice,
                 const char* filename,
                 vk::ShaderStageFlagBits stage);

    ShaderModule(ShaderModule&& other) noexcept
      : m_stage(other.m_stage),
        m_module(std::move(other.m_module))
    {}

    [[nodiscard]] vk::PipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

  private:
    vk::ShaderStageFlagBits m_stage{};
    vk::raii::ShaderModule m_module = nullptr;

    static std::vector<char> readFile(const char* filename);

    void createShaderModule(const std::shared_ptr<LogicalDevice>& logicalDevice,
                            const char* file);
  };

} // namespace vke

#endif //VKE_SHADERMODULE_H