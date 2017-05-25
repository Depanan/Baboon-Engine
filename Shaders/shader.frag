#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout (location = 3) in vec3 inViewVec;



layout(set = 0,binding = 2) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {

	vec3 ambient = vec3(0.2,0.2,0.2);//TODO: Should be material uniform
	vec3 spec = vec3(0.2,0.2,0.2);//TODO: Should be material uniform

	vec3 N = normalize(fragNormal);
	vec3 L = normalize(vec3(1.0,1.0,1.0));//TODO: Light dir should be uniform
	vec3 V = normalize(inViewVec);

	vec3 R = -normalize(reflect(-L, N));
	vec3 diffuse = max(dot(N, L), 0.0) *  vec3(1.0,1.0,1.0);
	vec3 specular = pow(max(dot(R, V), 0.0), 32.0) * spec;

    vec4 texColor = texture(texSampler, fragTexCoord);
	outColor = vec4((diffuse + ambient) * texColor.rgb + specular , 1.0);		
	//outColor = vec4(specular , 1.0);	
	//outColor = vec4(specular , 1.0);	
}