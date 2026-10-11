#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload {
  vec3 color;
  int depth;
  // Distance from the ray origin to the first surface hit, -1 where there is none.
  float hitDistance;
};

struct CloudHitAttribute {
  vec4 color;
  float exit;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT RayPayload payload;

hitAttributeEXT CloudHitAttribute cloudHit;

void main()
{
  vec3 result = cloudHit.color.xyz;

  int currentDepth = payload.depth;
  int MAX_DEPTH = 5;

  payload.hitDistance = -1.0;

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
      10000.0,
      0
    );

    result = mix(payload.color, result, cloudHit.color.a);

    // The cloud is translucent, so the depth is that of whatever is seen through it, measured from this ray's origin.
    if (payload.hitDistance >= 0.0)
    {
      payload.hitDistance += cloudHit.exit + 0.001;
    }
  }

  payload.color = result;
  payload.depth = currentDepth;
}