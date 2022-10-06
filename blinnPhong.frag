#version 460

layout (location = 0) smooth in vec3 vertexWorldNormal;
layout (location = 1) smooth in vec3 vertexWorldPos;
layout (location = 2) in vec2 textureCoords;

layout(set = 0, binding = 0) uniform FrameData{
	vec4 lightDirection;
	vec4 cameraPos;
} frameData;

layout(set = 2, binding = 0) uniform sampler2D textureSampler;

layout (location = 0) out vec4 outColor;

vec3 lightColor = normalize(vec3(0.8, 0.8, 0.8));//TODO move out to framedata uniform
float ambientStrength = 0.2;

void main()
{
 
	vec4 texColor = texture(textureSampler, textureCoords);
	vec3 objectColor = texColor.xyz;
	vec3 ambientColor = ambientStrength * lightColor;

	//pretending light direciton is position for now
	vec3 lightDirection = normalize(frameData.lightDirection.xyz - vertexWorldPos);
	float lightInfluence = max(dot(vertexWorldNormal, lightDirection), 0.0);
	vec3 diffuseColor = lightInfluence * lightColor;

	vec3 vertexToEye = normalize(frameData.cameraPos.xyz - vertexWorldPos);
	vec3 reflectedLight = reflect(-lightDirection, vertexWorldNormal);

	float specularInfluence = pow(max(dot(vertexToEye, reflectedLight), 0.0), 32);
	vec3 specularColor = 0.5 * specularInfluence * lightColor;

	vec3 result = (ambientColor + diffuseColor + specularColor) * objectColor;
	outColor = vec4(result, 1.0);

}