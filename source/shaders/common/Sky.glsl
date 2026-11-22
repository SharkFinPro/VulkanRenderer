#ifndef VKE_SKY_GLSL
#define VKE_SKY_GLSL

// The background for rays that hit nothing; the ray tracing miss shader and the raster sky pass both use it.
vec3 skyColor(vec3 direction)
{
  vec3 rayDir = normalize(direction);

  vec3 horizonColor = vec3(230, 242, 255) / 255.0f;
  vec3 zenithColor  = vec3(30, 125, 200) / 255.0f;

  float horizon = 1.0 - abs(rayDir.y);
  horizon = pow(horizon, 6.0);

  return mix(zenithColor, horizonColor, horizon);
}

#endif
