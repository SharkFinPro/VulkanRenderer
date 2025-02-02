#version 450

layout(set = 1, binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 0, binding = 4) uniform Curtain {
  float amplitude;
  float period;
  float shininess;
} curtain;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

const float PI = 3.14;

void main()
{
  fragPos = vec3(transform.model * vec4(inPosition, 1.0));
  fragTexCoord = inTexCoord;
//  fragNormal = mat3(transpose(inverse(transform.model))) * inNormal;

  float dzdx = curtain.amplitude * (5-inPosition.y) * (2.*PI/curtain.period) * cos( 2.*PI*inPosition.x/curtain.period );
  float dzdy = -curtain.amplitude * sin( 2.*PI*inPosition.x/curtain.period );
  vec3 Tx = vec3(1., 0., dzdx );
  vec3 Ty = vec3(0., 1., dzdy );
  fragNormal = normalize( cross( Tx, Ty ) );

  vec3 pos = inPosition;
  pos.z = sin(2 * PI * pos.x * curtain.period) * (5 - pos.y) * curtain.amplitude;

  gl_Position = transform.proj * transform.view * transform.model * vec4(pos, 1.0);
}