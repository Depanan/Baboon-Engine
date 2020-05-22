#version 320 es

layout(set = 0, binding = 1) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec3 camPos;
} ubo;
layout (push_constant) uniform PushConstants {
	mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout (location = 3) out vec3 fragPos;

#ifdef HAS_INCOLOR
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;
#endif
#ifdef HAS_INTEXCOORD
layout(location = 2) in vec2 inTexCoord;
layout(location = 1) out vec2 fragTexCoord;
#endif

#ifdef HAS_INNORMAL
layout(location = 3) in vec3 inNormal;
layout(location = 2) out vec3 fragNormal;
#endif

#ifdef HAS_INTANGENT
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBiTangent;
layout (location = 4) out mat3 TBN;
#endif





void main() {
	vec4 worldPos = pushConstants.model * vec4(inPosition, 1.0);
	fragPos = worldPos.xyz;
	
    gl_Position = ubo.proj * ubo.view * worldPos;
	#ifdef HAS_INCOLOR
	fragColor = inColor;
	#endif
	
	#ifdef HAS_INTEXCOORD
	fragTexCoord = inTexCoord;
	#endif
	
	#ifdef HAS_INNORMAL
	mat3 normalMatrix = mat3(transpose(inverse(pushConstants.model)));//TODO: pass normal matrix as ubo uniform...
	fragNormal =  normalMatrix *inNormal;
	#endif
	
	#ifdef HAS_INTANGENT
	vec3 T = normalize(vec3(pushConstants.model * vec4(normalMatrix * inTangent,   0.0)));
	vec3 B = normalize(vec3(pushConstants.model * vec4(normalMatrix * inBiTangent, 0.0)));
	vec3 N = normalize(vec3(pushConstants.model * vec4(fragNormal,    0.0)));
	TBN = mat3(T, B, N);
	#endif
	
}