#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#include "../common/Sky.glsl"

struct RayPayload {
  vec3 color;
  int depth;
  float hitDistance;
};

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main()
{
  payload.color = skyColor(gl_WorldRayDirectionEXT);
}
