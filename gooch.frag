#version 450

layout(location = 0) out vec4 outFragColor;
layout(location = 0) in vec3 color;
layout(location = 1) in vec3 normal;
//layout(binding = 0) uniform vec3 lightDirection;

void main()
{
	vec4 coolColor = normalize(vec4(0.07, 0.6, 0.9, 1.0));
	vec4 hotColor = normalize(vec4(0.8, 0.1, 0.4, 1.0));
	vec3 light = normalize(vec3(0.0,-2.0,0.0));
	//vec3 nLight = normalize(lightDirection);
	vec3 nNormal = normalize(normal);
	//shaded = (1-s)(tWarm + (1-t)cool)
	//TODO diffuse only, add highlights
		//vec3 r = reflect(light, normal);
	float lightAngle = dot(nNormal, light);
	float t = (lightAngle + 1.0) / 2.0;
	outFragColor = t * hotColor + (1-t) * coolColor;
}