#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

layout(location = 0) rayPayloadInEXT vec3 payload;

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

  vec3 normal   = normalize(bary.x * v0.normal   + bary.y * v1.normal   + bary.z * v2.normal);
  vec2 texCoord =           bary.x * v0.texCoord + bary.y * v1.texCoord + bary.z * v2.texCoord;

  // Debug: normals as color
  payload = normal * 0.5 + 0.5;

  // Debug: UVs as color
  // payload = vec3(texCoord, 0.0);
}