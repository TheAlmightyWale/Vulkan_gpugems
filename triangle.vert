#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(push_constant) uniform PushConstants {
	mat4 mvp;
} pcs;

layout(location = 0) out vec4 color;

void main()
{
	gl_Position = pcs.mvp * vec4(position, 1.0);
	color = vec4(normal * 0.5 + vec3(0.5), 1.0);
}