#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
  vec3 color;
  int depth;
};

struct SmokeHitAttribute {
  vec4 color;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT SmokeHitAttribute smokeHit;

void main()
{
  vec3 result = smokeHit.color.xyz;

  payload.color = result;
}