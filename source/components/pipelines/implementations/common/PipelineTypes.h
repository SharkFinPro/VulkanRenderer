#ifndef VULKANPROJECT_PIPELINETYPES_H
#define VULKANPROJECT_PIPELINETYPES_H

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

#endif //VULKANPROJECT_PIPELINETYPES_H