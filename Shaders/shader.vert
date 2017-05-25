#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SceneUniforms {
    mat4 view;
    mat4 proj;
} sceneUbo;

layout (set = 0, binding = 1) uniform UboInstance 
{
	mat4 model; 
} instanceUbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout (location = 3) out vec3 outViewVec;


void main() {
	vec4 worldPos = instanceUbo.model * vec4(inPosition, 1.0);
	outViewVec = -worldPos.xyz;

	

	gl_Position = sceneUbo.proj * sceneUbo.view * worldPos;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
	fragNormal = mat3(transpose(inverse(instanceUbo.model))) * inNormal;//TODO: pass normal matrix as ubo uniform...
	

	

}
