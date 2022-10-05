#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureCoords;

//TODO includes / pre-build step so we can factor out duplicate code
struct ObjectData {
	mat4 transform;
};

struct CameraData {
	mat4 viewProj;
};

layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer {
	CameraData camera;
	ObjectData objects[];
} objectBuffer;


layout(location = 0) out vec3 vertexWorldNormal;
layout(location = 1) out vec2 otextureCoords;

void main()
{
	mat4 transform = objectBuffer.camera.viewProj * objectBuffer.objects[gl_BaseInstance].transform;
	gl_Position = transform * vec4(position, 1.0);
	vertexWorldNormal = (objectBuffer.objects[gl_BaseInstance].transform * vec4(normal, 0.0)).xyz;
	otextureCoords = textureCoords;
}

