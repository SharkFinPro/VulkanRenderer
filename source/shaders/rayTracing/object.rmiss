#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 payload;

void main()
{
  vec3 rayDir = normalize(gl_WorldRayDirectionEXT);

  float t = clamp(rayDir.y, 0.0, 1.0);

  vec3 horizonColor = vec3(230, 242, 255) / 255.0f;
  vec3 zenithColor  = vec3(30, 125, 200) / 255.0f;

  float horizon = 1.0 - abs(rayDir.y);
  horizon = pow(horizon, 6.0);

  payload = mix(zenithColor, horizonColor, horizon);
}