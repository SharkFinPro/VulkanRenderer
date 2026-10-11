#ifndef VKE_PIPELINETYPES_H
#define VKE_PIPELINETYPES_H

namespace vke {

  enum class PipelineType {
    bumpyCurtain,
    crosses,
    curtain,
    cubeMap,
    ellipticalDots,
    magnifyWhirlMosaic,
    noisyEllipticalDots,
    object,
    texturedPlane,
    snake,

    ellipse,
    font,
    grid,
    mousePicking,
    outline,
    outlineMask,
    rayTracingComposite,
    offscreenToSwapchain,
    pointLightShadowMap,
    rect,
    shadow,
    triangle
  };

  // View-only pipelines draw aids the ray tracer does not model, drawn by raster over the traced image.
  [[nodiscard]] constexpr bool isViewOnlyPipeline(const PipelineType pipelineType)
  {
    return pipelineType == PipelineType::texturedPlane;
  }

} // namespace vke

#endif //VKE_PIPELINETYPES_H
