#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier  : require
#include "../common/Lighting.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT vec3 payload;

layout(location = 1) rayPayloadEXT bool isShadowed;

hitAttributeEXT vec2 barycentrics;

struct Vertex {
  vec3 pos;
  float padding1;
  vec3 normal;
  float padding2;
  vec2 texCoord;
  vec2 padding3;
};

struct MeshInfo {
  uint vertexOffset;
  uint indexOffset;
  uint textureIndex;
  uint padding;
};

layout(binding = 3, set = 0) readonly buffer VertexBuffer {
  Vertex vertices[];
};

layout(binding = 4, set = 0) readonly buffer IndexBuffer {
  uint indices[];
};

layout(binding = 5, set = 0) readonly buffer MeshInfoBuffer {
  MeshInfo meshInfos[];
};

layout(binding = 6, set = 0) uniform sampler2D textures[];

layout(set = 1, binding = 0) uniform PointLightsMetadata {
  int numPointLights;
  int numSpotLights;
};

layout(set = 1, binding = 1) readonly buffer PointLights {
  PointLight pointLights[];
};

layout(set = 1, binding = 2) readonly buffer SpotLights {
  SpotLight spotLights[];
};

layout(set = 1, binding = 3) uniform Camera {
  vec3 position;
} camera;

void main()
{
  vec3 bary = vec3(1.0 - barycentrics.x - barycentrics.y,
  barycentrics.x,
  barycentrics.y);

  MeshInfo info = meshInfos[gl_InstanceCustomIndexEXT];

  uint i0 = indices[info.indexOffset + gl_PrimitiveID * 3 + 0];
  uint i1 = indices[info.indexOffset + gl_PrimitiveID * 3 + 1];
  uint i2 = indices[info.indexOffset + gl_PrimitiveID * 3 + 2];

  Vertex v0 = vertices[info.vertexOffset + i0];
  Vertex v1 = vertices[info.vertexOffset + i1];
  Vertex v2 = vertices[info.vertexOffset + i2];

  vec3 normal = normalize(bary.x * v0.normal   + bary.y * v1.normal   + bary.z * v2.normal);
  vec2 texCoord = bary.x * v0.texCoord + bary.y * v1.texCoord + bary.z * v2.texCoord;

  // Debug: normals as color
//  payload = normal * 0.5 + 0.5;

  // Debug: UVs as color
//  payload = vec3(texCoord, 0.0);

  vec3 texColor = texture(textures[nonuniformEXT(info.textureIndex)], texCoord).rgb;
  vec3 specColor = vec3(0);

  vec3 localPos = bary.x * v0.pos + bary.y * v1.pos + bary.z * v2.pos;
  vec3 fragPos = vec3(gl_ObjectToWorldEXT * vec4(localPos, 1.0));

  vec3 fragNormal = normalize(normal * mat3(gl_WorldToObjectEXT));

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    PointLight light = pointLights[i];

    vec3 toLight = light.position - fragPos;
    float lightDist = length(toLight);

    isShadowed = true;
    traceRayEXT(
      tlas,
      gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
      0xFF,
      0,
      0,
      1,
      fragPos,
      0.001,
      normalize(toLight),
      lightDist - 0.001,
      1
    );

    if (!isShadowed)
    {
      result += SpecularMapPointLightAffect(light, texColor, specColor, fragNormal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(light.ambient, texColor);
    }
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    SpotLight light = spotLights[i];

    vec3 toLight = light.position - fragPos;
    float lightDist = length(toLight);

    isShadowed = true;
    traceRayEXT(
      tlas,
      gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
      0xFF,
      0,
      0,
      1,
      fragPos,
      0.001,
      normalize(toLight),
      lightDist - 0.001,
      1
    );

    if (!isShadowed)
    {
      result += SpecularMapSpotLightAffect(light, texColor, specColor, fragNormal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(light.ambient, texColor);
    }
  }

  payload = result;
}