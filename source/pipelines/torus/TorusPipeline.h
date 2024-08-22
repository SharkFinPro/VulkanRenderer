#ifndef TORUSPIPELINE_H
#define TORUSPIPELINE_H

#include <cstdint>
#include <deque>

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

constexpr uint32_t numU = 1024;
constexpr uint32_t numV = 1024;

inline void generateIndices(uint32_t* indices)
{
  for (uint32_t j = 0; j < numV - 1; j++)
  {
    for (uint32_t i = 0; i < numU - 1; i++)
    {
      const uint32_t offset = (j * (numU - 1) + i) * 6;
      const uint32_t i1 = (j + 0) * numU + (i + 0);
      const uint32_t i2 = (j + 0) * numU + (i + 1);
      const uint32_t i3 = (j + 1) * numU + (i + 1);
      const uint32_t i4 = (j + 1) * numU + (i + 0);

      indices[offset + 0] = i1;
      indices[offset + 1] = i2;
      indices[offset + 2] = i4;
      indices[offset + 3] = i2;
      indices[offset + 4] = i3;
      indices[offset + 5] = i4;
    }
  }
}

class TorusPipeline : public ComputePipeline, public GraphicsPipeline {
public:
  TorusPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice, const std::shared_ptr<LogicalDevice>& logicalDevice,
                const VkCommandPool& commandPool, const VkRenderPass& renderPass, const VkExtent2D& swapChainExtent);

private:
  void initMesh();

private:
  std::deque<std::pair<uint32_t, uint32_t>> morphQueue = { {5, 8 }, {5, 8} }
  float morphCoef = 0.0f;
  float animationSpeed = 1.0f;
};



#endif //TORUSPIPELINE_H
