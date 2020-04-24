#version 450

layout(set = 0, binding = 1) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;
layout (push_constant) uniform PushConstants {
	mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout (location = 3) out vec3 outViewVec;

void main() {
	vec4 worldPos = pushConstants.model * vec4(inPosition, 1.0);
	outViewVec = -worldPos.xyz;
	
    gl_Position = ubo.proj * ubo.view * worldPos;
	fragColor = inColor;
	fragTexCoord = inTexCoord;
	fragNormal = mat3(transpose(inverse(pushConstants.model))) * inNormal;//TODO: pass normal matrix as ubo uniform...
}