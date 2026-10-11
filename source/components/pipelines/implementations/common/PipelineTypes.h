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
    mousePickingAlphaTested,
    outline,
    outlineMask,
    outlineSingleSample,
    offscreenToSwapchain,
    pointLightShadowMap,
    rect,
    shadow,
    triangle
  };

  // Whether the pipeline discards texels below the alpha cutoff, so picking must cut out the same shape.
  [[nodiscard]] constexpr bool usesAlphaCutout(const PipelineType pipelineType)
  {
    return pipelineType == PipelineType::texturedPlane;
  }

} // namespace vke

#endif //VKE_PIPELINETYPES_H
