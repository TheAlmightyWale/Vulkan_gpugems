#version 450

layout(location = 0) in vec2 uv;
layout(binding = 0) uniform sampler2D fontSampler;

layout(location = 0) out vec4 oColor;

void main()
{
	float color = texture(fontSampler, uv).r;
	oColor = vec4(color); 
}