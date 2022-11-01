#version 450

layout(location = 0) in vec3 position;

layout(push_constant) uniform PushConstants {
	mat4 mvp;
} pcs;

layout(location = 0) out vec4 color;

void main()
{
	gl_Position = pcs.mvp * vec4(position, 1.0);
	color = vec4(vec3(0.8, 0.0, 0.0), 1.0);
}