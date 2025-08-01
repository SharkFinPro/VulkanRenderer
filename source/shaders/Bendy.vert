#version 450

layout(set = 0, binding = 0) uniform Transform {
  mat4 mvp;
} transform;

layout(set = 0, binding = 1) uniform Bendy {
  float pitch;
  float bendStrength;
  float time;
} bendy;

layout(location = 0) out vec2 fragTexCoord;

const float YAW_DEGREES_PER_INSTANCE = 65.0;
const float PITCH_OFFSET_PER_INSTANCE = 2.5;

void main()
{
  // Generate quad vertices from vertex index (0,1,2,3 -> corners of quad)
  vec2 quadVertex = vec2(
    float(gl_VertexIndex % 2) - 0.5,  // x: -0.5 or 0.5
    float(gl_VertexIndex / 2)         // y: 0.0 or 1.0
  );

  fragTexCoord = vec2(
    (gl_VertexIndex % 2) / 2.0 + 0.25,
    quadVertex.y / 4.0
  );

  // Calculate rotation angles
  float yawRadians = radians(YAW_DEGREES_PER_INSTANCE * float(gl_InstanceIndex));
  float pitchRadians = radians(bendy.pitch - float(gl_InstanceIndex) * PITCH_OFFSET_PER_INSTANCE);

  // Sway
  pitchRadians += radians(sin(bendy.time) * 5.0);

  // Pre-calculate trigonometric values
  vec2 yawTrig = vec2(cos(yawRadians), sin(yawRadians));  // cos, sin

  // Apply bending effect to pitch based on vertex height
  float bendPitchRadians = pitchRadians + quadVertex.y * bendy.bendStrength;
  vec2 bendPitchTrig = vec2(cos(bendPitchRadians), sin(bendPitchRadians));  // cos, sin

  // Calculate 3D position with rotations applied
  vec3 position = vec3(
    yawTrig.x * -quadVertex.x + bendPitchTrig.x * quadVertex.y * yawTrig.y,  // x
    bendPitchTrig.y * quadVertex.y,                                          // y
    yawTrig.y * quadVertex.x + bendPitchTrig.x * quadVertex.y * yawTrig.x    // z
  );

  // Transform to clip space
  gl_Position = transform.mvp * vec4(position, 1.0);
}
