#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference : require
#include "../common/Lighting.glsl"
#include "../common/VolumePalette.glsl"

struct RayPayload {
  vec3 color;
  int depth;
  // Distance from the ray origin to the first surface hit, -1 where there is none.
  float hitDistance;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT RayPayload payload;

layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec2 barycentrics;

// Mirrors VolumeVertex, spelled out in scalars so std430 gives the same 32-byte layout
struct VolumeVertex {
  float px, py, pz;
  float nx, ny, nz;
  uint materials;
  uint weights;
};

layout(buffer_reference, std430) readonly buffer VertexRef {
  VolumeVertex vertices[];
};

layout(buffer_reference, std430) readonly buffer IndexRef {
  uint indices[];
};

struct VolumeChunkInfo {
  VertexRef vertexBuffer;
  IndexRef indexBuffer;
};

layout(binding = 8, set = 0) readonly buffer VolumeChunkBuffer {
  VolumeChunkInfo volumeChunks[];
};

layout(set = 1, binding = 0) uniform PointLightsMetadata {
  int numPointLights;
  int numSpotLights;
  int numDirectionalLights;
};

layout(set = 1, binding = 1) readonly buffer PointLights {
  PointLight pointLights[];
};

layout(set = 1, binding = 2) readonly buffer SpotLights {
  SpotLight spotLights[];
};

layout(set = 1, binding = 6) readonly buffer DirectionalLights {
  DirectionalLight directionalLights[];
};

layout(set = 1, binding = 3) uniform Camera {
  vec3 position;
} camera;

// Starts off the surface along the normal facing the ray, since facing culling is off and a ray from the hit point could
// hit its own triangle again
bool isOccluded(vec3 fragPos, vec3 normal, vec3 lightPosition)
{
  const vec3 origin = fragPos + normal * 0.01;
  const vec3 toLight = lightPosition - origin;

  isShadowed = true;
  traceRayEXT(
    tlas,
    gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
    0x02,
    0,
    0,
    1,
    origin,
    0.001,
    normalize(toLight),
    length(toLight) - 0.001,
    1
  );

  return isShadowed;
}

// A directional light has no position: the ray along the light vector has no end
bool isOccludedAlongDirection(vec3 fragPos, vec3 normal, vec3 toLight)
{
  const vec3 origin = fragPos + normal * 0.01;

  isShadowed = true;
  traceRayEXT(
    tlas,
    gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
    0x02,
    0,
    0,
    1,
    origin,
    0.001,
    toLight,
    1.0e30,
    1
  );

  return isShadowed;
}

void main()
{
  const vec3 bary = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

  VolumeChunkInfo chunk = volumeChunks[gl_InstanceCustomIndexEXT];

  const uint i0 = chunk.indexBuffer.indices[gl_PrimitiveID * 3 + 0];
  const uint i1 = chunk.indexBuffer.indices[gl_PrimitiveID * 3 + 1];
  const uint i2 = chunk.indexBuffer.indices[gl_PrimitiveID * 3 + 2];

  const VolumeVertex v0 = chunk.vertexBuffer.vertices[i0];
  const VolumeVertex v1 = chunk.vertexBuffer.vertices[i1];
  const VolumeVertex v2 = chunk.vertexBuffer.vertices[i2];

  const vec3 localPos = bary.x * vec3(v0.px, v0.py, v0.pz) + bary.y * vec3(v1.px, v1.py, v1.pz) + bary.z * vec3(v2.px, v2.py, v2.pz);
  const vec3 localNormal = bary.x * vec3(v0.nx, v0.ny, v0.nz) + bary.y * vec3(v1.nx, v1.ny, v1.nz) + bary.z * vec3(v2.nx, v2.ny, v2.nz);

  const vec3 fragPos = vec3(gl_ObjectToWorldEXT * vec4(localPos, 1.0));

  vec3 normal = normalize(localNormal * mat3(gl_WorldToObjectEXT));

  // Double-sided: a back face (the surface seen from inside the solid) is lit as its other side. Which side the ray hit is
  // the hit kind's to say, as gl_FrontFacing says in the raster shader (VolumeSurface.frag): judged from the shading
  // normal, a face in front of the ray whose interpolated normal leans away from it (a thin lip) had its normal flipped to
  // face away from every light.
  if (gl_HitKindEXT == gl_HitKindBackFacingTriangleEXT)
  {
    normal = -normal;
  }

  // The first corner's own material (the one with the full weight), as the raster vertex stage passes on
  uint material = v0.materials & 0xFFu;
  for (uint i = 1; i < 4; i++)
  {
    if (((v0.weights >> (8 * i)) & 0xFFu) > (v0.weights & 0xFFu))
    {
      material = (v0.materials >> (8 * i)) & 0xFFu;
    }
  }

  const vec3 albedo = volumePaletteAlbedo(material);

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    const PointLight light = pointLights[i];

    float attenuation = lightAttenuation(light.range, light.position, fragPos);
    if (attenuation <= 0.0)
    {
      continue;
    }

    if (facesLight(normal, fragPos, light.position) && !isOccluded(fragPos, normal, light.position))
    {
      result += StandardPointLightAffect(light, albedo, normal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(light.ambient, albedo) * attenuation;
    }
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    const SpotLight light = spotLights[i];

    float attenuation = lightAttenuation(light.range, light.position, fragPos);
    if (attenuation <= 0.0)
    {
      continue;
    }

    if (facesLight(normal, fragPos, light.position) && !isOccluded(fragPos, normal, light.position))
    {
      result += StandardSpotLightAffect(light, albedo, normal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(light.ambient, albedo) * attenuation;
    }
  }

  for (int i = 0; i < numDirectionalLights; i++)
  {
    const DirectionalLight light = directionalLights[i];

    if (facesDirectionalLight(normal, light.direction) && !isOccludedAlongDirection(fragPos, normal, -light.direction))
    {
      result += StandardDirectionalLightAffect(light, albedo, normal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(light.ambient, albedo);
    }
  }

  payload.color = result;
  payload.hitDistance = gl_HitTEXT;
}
