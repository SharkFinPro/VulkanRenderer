// Scene set (set 0): the per-frame object table, draw slots and camera, shared by every object pipeline. A draw's first
// instance indexes drawSlots, whose entry is the object's slot in the table, so one instanced draw covers several
// objects. Vertex shaders get their object's slot with objectSlot(); later stages receive it as a flat varying.
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

layout(std430, set = 0, binding = 2) readonly buffer DrawSlots {
  uint drawSlots[];
};

// A macro rather than a function, so including this in a stage without gl_InstanceIndex still compiles.
#define objectSlot() drawSlots[gl_InstanceIndex]

layout(std140, set = 0, binding = 1) uniform SceneCamera {
  mat4 view;
  mat4 proj;
} scene;
