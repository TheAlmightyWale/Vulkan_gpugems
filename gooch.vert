#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

struct ObjectData {
 mat4 transform;
};

//Packing of a buffer occurs along 16 byte alignment with std140
// generally avoid using vec3 and std140
layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 oNormal;

void main()
{
	mat4 transform = objectBuffer.objects[gl_BaseInstance].transform;
	gl_Position = transform * vec4(position, 1.0);
	color = vec3(0.2,0.2,0.2);
	oNormal = normal;
}