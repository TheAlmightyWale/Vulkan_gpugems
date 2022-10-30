#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 _unused;

struct ObjectData {
	mat4 transform;
};

struct CameraData {
	mat4 viewProj;
};

//Packing of a buffer occurs along 16 byte alignment with std140, can be packed tighter with std430
// generally avoid using vec3 and std430
layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer {
	CameraData camera;
	ObjectData objects[];
} objectBuffer;

layout(location = 0) out vec3 vertexWorldNormal;

void main()
{
	mat4 transform = objectBuffer.camera.viewProj * objectBuffer.objects[gl_BaseInstance].transform;
	gl_Position = transform * vec4(position, 1.0);
	vertexWorldNormal = (objectBuffer.objects[gl_BaseInstance].transform * vec4(normal, 0.0)).xyz;
}