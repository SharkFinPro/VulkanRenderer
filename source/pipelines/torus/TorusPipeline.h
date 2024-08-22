#ifndef TORUSPIPELINE_H
#define TORUSPIPELINE_H

#include <cstdint>

#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"

struct MeshUniformBuffer {
   float time;
   uint32_t numU;
   uint32_t numV;
   float minU, maxU;
   float minV, maxV;
   uint32_t p1, p2;
   uint32_t q1, q2;
   float morph;
};

class TorusPipeline : public ComputePipeline, public GraphicsPipeline {
public:
  TorusPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice, const std::shared_ptr<LogicalDevice>& logicalDevice,
                const VkCommandPool& commandPool, const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent);

private:
};



#endif //TORUSPIPELINE_H
