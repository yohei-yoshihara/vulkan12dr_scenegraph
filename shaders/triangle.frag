#version 450
layout(location = 0) in vec3 in_color;
layout(location = 1) in float in_lightIntensity;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(in_color * in_lightIntensity, 0.2);
}