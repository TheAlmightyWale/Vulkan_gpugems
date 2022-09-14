#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(push_constant) uniform PushConstants {
	mat4 mvp;
} pcs;

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 oNormal;

void main()
{
	gl_Position = pcs.mvp * vec4(position, 1.0);
	color = vec3(0.2,0.2,0.2);
	oNormal = normal;
}