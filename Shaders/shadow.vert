#version 450

layout(location = 0) in vec3 inPosition;

layout (location = 0) out int outInstanceIndex;


layout (push_constant) uniform PushConstants {
	mat4 model;
} pushConstants;


void main()
{
    gl_Position = pushConstants.model * vec4(inPosition, 1.0);
}