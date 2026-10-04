#version 450

layout(push_constant) uniform PushConstants {
  uint maskValue;
};

layout(location = 0) out uint outMask;

void main()
{
  outMask = maskValue;
}
