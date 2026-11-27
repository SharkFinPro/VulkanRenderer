#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require
#include "../common/Lighting.glsl"
#include "../common/VolumeMaterial.glsl"

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

// The palette's uniform block (VolumePaletteData), read through its device address
layout(buffer_reference, std430) readonly buffer PaletteRef {
  vec4 textureScales[maxVolumeMaterials];
  uint materialCount;
};

// Mirrors VolumeChunkInfo in RayTracer.h: 32 bytes
struct VolumeChunkInfo {
  VertexRef vertexBuffer;
  IndexRef indexBuffer;
  PaletteRef palette;
  uint paletteIndex;
  uint padding;
};

layout(binding = 8, set = 0) readonly buffer VolumeChunkBuffer {
  VolumeChunkInfo volumeChunks[];
};

// Albedo and specular arrays of this frame's palettes: palette i is at 2i and 2i + 1
layout(binding = 9, set = 0) uniform sampler2DArray volumePaletteArrays[64];

layout(binding = 2, set = 0) uniform RayTracingCamera {
  mat4 viewInverse;
  mat4 projInverse;
  mat4 projection;
  vec3 position;
  float nearPlane;
  float farPlane;
} rtCamera;

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

  // A triangle's corners list the same materials, so the first corner's say which; the weights blend by barycentrics
  const uint materials = v0.materials;
  vec4 weights = vec4(0.0);
  for (uint i = 0; i < 4; i++)
  {
    weights[i] = bary.x * float((v0.weights >> (8 * i)) & 0xFFu) + bary.y * float((v1.weights >> (8 * i)) & 0xFFu) +
                 bary.z * float((v2.weights >> (8 * i)) & 0xFFu);
  }
  weights /= 255.0;

  const vec3 axisWeights = triplanarWeights(normal);

  // An approximation of the mip level from a ray cone: the pixel's spread angle over the hit distance, widened where
  // the surface is seen at a grazing angle. Exact for primary rays; a secondary ray only knows its own hit distance.
  const float spread = 2.0 / (abs(rtCamera.projection[1][1]) * float(gl_LaunchSizeEXT.y));
  const float footprint = gl_HitTEXT * spread / max(abs(dot(normal, gl_WorldRayDirectionEXT)), 0.25);

  const uint albedoArrayIndex = 2 * chunk.paletteIndex;
  const uint specularArrayIndex = albedoArrayIndex + 1;
  const vec2 albedoSize = vec2(max(textureSize(volumePaletteArrays[nonuniformEXT(albedoArrayIndex)], 0).xy, ivec2(1)));
  const vec2 specularSize = vec2(max(textureSize(volumePaletteArrays[nonuniformEXT(specularArrayIndex)], 0).xy, ivec2(1)));

  vec3 albedo = vec3(0.0);
  vec3 specColor = vec3(0.0);
  float totalWeight = 0.0;

  for (int slot = 0; slot < 4; slot++)
  {
    const float materialWeight = weights[slot];
    if (materialWeight <= 0.0)
    {
      continue;
    }

    const uint layerIndex = ((materials >> (8 * slot)) & 0xFFu) % chunk.palette.materialCount;
    const float layer = float(layerIndex);
    const float scale = chunk.palette.textureScales[layerIndex].x;

    for (int axis = 0; axis < 3; axis++)
    {
      if (axisWeights[axis] <= 0.0)
      {
        continue;
      }

      const vec2 uv = projectAxis(axis, fragPos) * scale;
      const float weight = materialWeight * axisWeights[axis];
      const float albedoLod = log2(max(footprint * scale * max(albedoSize.x, albedoSize.y), 1e-6));
      const float specularLod = log2(max(footprint * scale * max(specularSize.x, specularSize.y), 1e-6));

      albedo += weight * textureLod(volumePaletteArrays[nonuniformEXT(albedoArrayIndex)], vec3(uv, layer), albedoLod).rgb;
      specColor += weight * textureLod(volumePaletteArrays[nonuniformEXT(specularArrayIndex)], vec3(uv, layer), specularLod).rgb;
    }

    totalWeight += materialWeight;
  }

  albedo = totalWeight > 0.0 ? albedo / totalWeight : vec3(1.0);
  specColor = totalWeight > 0.0 ? specColor / totalWeight : vec3(1.0);

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
      result += SpecularMapPointLightAffect(light, albedo, specColor, normal, fragPos, camera.position, 32);
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
      result += SpecularMapSpotLightAffect(light, albedo, specColor, normal, fragPos, camera.position, 32);
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
      result += SpecularMapDirectionalLightAffect(light, albedo, specColor, normal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(light.ambient, albedo);
    }
  }

  payload.color = result;
  payload.hitDistance = gl_HitTEXT;
}
