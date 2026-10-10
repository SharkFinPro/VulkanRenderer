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
    outlineSingleSample,
    offscreenToSwapchain,
    pointLightShadowMap,
    rect,
    shadow,
    triangle
  };

} // namespace vke

#endif //VKE_PIPELINETYPES_H
