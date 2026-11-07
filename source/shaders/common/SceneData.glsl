// Scene set (set 0): the per-frame object table and camera, shared by every object pipeline. An object's slot in the
// table is passed as the draw's first instance, so gl_InstanceIndex in the vertex stage is the index into objects.
// Keep ObjectData in step with the C++ struct in ObjectTable.h (144 bytes).
struct ObjectData {
  mat4 model;
  mat4 normalMatrix; // mat4(transpose(inverse(mat3(model))))
  uint pickId;       // 1-based picking id, or 0 when the submission is not pickable
  uint pad0;
  uint pad1;
  uint pad2;
};

layout(std430, set = 0, binding = 0) readonly buffer ObjectTable {
  ObjectData objects[];
};

layout(std140, set = 0, binding = 1) uniform SceneCamera {
  mat4 view;
  mat4 proj;
} scene;
