#version 450

layout(location = 0) out vec4 outFragColor;
layout(location = 1) in vec3 vertexWorldNormal;

layout(set = 0, binding = 0) uniform Light{
	vec3 direction;
} light;

void main()
{
	vec4 coolColor = normalize(vec4(0.07, 0.6, 0.9, 1.0));
	vec4 hotColor = normalize(vec4(0.8, 0.1, 0.4, 1.0));
	vec3 nLight = normalize(-light.direction);
	vec3 nNormal = normalize(vertexWorldNormal);
	//shaded = (1-s)(tWarm + (1-t)cool)
	//TODO diffuse only, add highlights
		//vec3 r = reflect(light, normal);
	float lightAngle = dot(nNormal, nLight);
	float t = (lightAngle + 1.0) / 2.0;
	outFragColor = t * hotColor + (1-t) * coolColor;
}