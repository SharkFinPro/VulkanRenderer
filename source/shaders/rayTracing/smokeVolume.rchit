#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
  vec3 color;
  int depth;
};

struct SmokeHitAttribute {
  vec4 color;
  float exit;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT SmokeHitAttribute smokeHit;

void main()
{
  vec3 result = smokeHit.color.xyz;

  int currentDepth = payload.depth;
  int MAX_DEPTH = 5;

  if (currentDepth < MAX_DEPTH)
  {
    payload.depth = currentDepth + 1;
    traceRayEXT(
      tlas,
      gl_RayFlagsOpaqueEXT | gl_RayFlagsCullBackFacingTrianglesEXT,
      0xFF,
      0,
      0,
      0,
      gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * (smokeHit.exit + 0.001),
      0.001,
      gl_WorldRayDirectionEXT,
      10000.0,
      0
    );

    result = mix(payload.color, result, smokeHit.color.a);
  }

  payload.color = result;
  payload.depth = currentDepth;
}