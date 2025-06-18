#version 450

layout(binding = 0) uniform UniformBufferObject {
  vec4 light;
  mat4 mvpMatrix;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;

layout(location = 0) out vec3 out_color;
layout(location = 1) out float out_lightIntensity;

void main()
{
  gl_Position = ubo.mvpMatrix * vec4(in_position, 1.0);
  out_lightIntensity = max(0.0, dot(in_normal, ubo.light.xyz)) + ubo.light.w;
  out_color = in_color;
}