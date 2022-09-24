#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(set = 1, binding = 0) uniform Transform {
	mat4 worldPos;
} transform;

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 oNormal;

void main()
{
	gl_Position = transform.worldPos * vec4(position, 1.0);
	color = vec3(0.2,0.2,0.2);
	oNormal = normal;
}