#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class LogicalDevice;

class Pipeline {
public:
  explicit Pipeline(const std::shared_ptr<LogicalDevice>& logicalDevice);

  virtual ~Pipeline();

  virtual void displayGui() {}

protected:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;

  std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
};



#endif //PIPELINE_H
