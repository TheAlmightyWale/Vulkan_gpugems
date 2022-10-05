#version 460

layout (location = 0) in vec3 vertexWorldNormal;
layout (location = 1) in vec2 textureCoords;

layout(set = 0, binding = 0) uniform Light{
	vec3 direction;
} light;

layout(set = 2, binding = 0) uniform sampler2D textureSampler;

layout (location = 0) out vec4 outColor;

void main()
{
	vec4 baseColor = texture(textureSampler, textureCoords); 
	outColor = baseColor;
}