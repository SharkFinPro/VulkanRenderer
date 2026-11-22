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
    volumeSurface,

    ellipse,
    font,
    grid,
    mousePicking,
    mousePickingAlphaTested,
    outline,
    outlineMask,
    rayTracingComposite,
    sky,
    offscreenToSwapchain,
    pointLightShadowMap,
    rect,
    shadow,
    triangle,
    volumeSurfaceShadow,
    volumeSurfacePointLightShadowMap,
    volumeSurfaceMousePicking,
    volumeSurfaceOutlineMask
  };

  // View-only pipelines draw aids the ray tracer does not model, drawn by raster over the traced image.
  [[nodiscard]] constexpr bool isViewOnlyPipeline(const PipelineType pipelineType)
  {
    return pipelineType == PipelineType::texturedPlane;
  }

  // The main pass pipeline of volume surfaces and its variants for the shadow, picking and outline mask passes. Surfaces
  // are drawn with Renderer3D::renderVolumeSurface, not renderObject.
  [[nodiscard]] constexpr bool isVolumeSurfacePipeline(const PipelineType pipelineType)
  {
    return pipelineType == PipelineType::volumeSurface ||
           pipelineType == PipelineType::volumeSurfaceShadow ||
           pipelineType == PipelineType::volumeSurfacePointLightShadowMap ||
           pipelineType == PipelineType::volumeSurfaceMousePicking ||
           pipelineType == PipelineType::volumeSurfaceOutlineMask;
  }

  // Whether the pipeline discards texels below the alpha cutoff, so picking must cut out the same shape.
  [[nodiscard]] constexpr bool usesAlphaCutout(const PipelineType pipelineType)
  {
    return pipelineType == PipelineType::texturedPlane;
  }

} // namespace vke

#endif //VKE_PIPELINETYPES_H
