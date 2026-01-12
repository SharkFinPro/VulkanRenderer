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
    objectHighlight,
    texturedPlane,
    snake,

    ellipse,
    font,
    gui,
    grid,
    mousePicking,
    pointLightShadowMap,
    rect,
    shadow,
    triangle
  };

} // namespace vke

#endif //VKE_PIPELINETYPES_H
