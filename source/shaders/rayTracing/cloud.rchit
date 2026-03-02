#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
  vec3 color;
  int depth;
};

struct CloudHitAttribute {
  vec3 color;
  float exit;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT CloudHitAttribute cloudHit;

void main()
{
  vec3 result = cloudHit.color;

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
      gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * (cloudHit.exit + 0.001),
      0.001,
      gl_WorldRayDirectionEXT,
      1000.0,
      0
    );

    result = mix(result, payload.color, 0.2);
  }

  payload.color = result;
  payload.depth = currentDepth;
}