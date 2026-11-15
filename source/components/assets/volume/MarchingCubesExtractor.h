#ifndef VKE_MARCHINGCUBESEXTRACTOR_H
#define VKE_MARCHINGCUBESEXTRACTOR_H

#include "SurfaceExtractor.h"

namespace vke {

  // Marching Cubes over the samples' zero set, a sample being inside when its distance is negative. One vertex per
  // crossing edge, welded within a call. Every crossing edge is named by its lower endpoint and axis and its vertex is
  // computed from those two samples alone, so any two calls that see an edge produce the same vertex bit for bit, which
  // is what makes neighboring chunks fit. Triangles wind counter-clockwise seen from outside.
  class MarchingCubesExtractor final : public SurfaceExtractor {
  public:
    void extract(const VolumeSamples& samples,
                 const CellRange& cells,
                 float cellSize,
                 SurfaceMesh& out) const override;
  };

} // namespace vke

#endif //VKE_MARCHINGCUBESEXTRACTOR_H
