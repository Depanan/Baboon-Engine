#version 450
#extension GL_ARB_separate_shader_objects : enable


layout (set=0, binding=0) uniform sampler2D baseTexture;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {

	vec4 base_color = vec4(1.0, 1.0, 0.0, 1.0);


    base_color = texture(baseTexture, fragTexCoord);

    outColor = vec4(base_color.rgb, 1.0);
}