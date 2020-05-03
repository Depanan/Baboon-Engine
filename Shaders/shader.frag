#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef HAS_BASETEXTURE
layout (set=0, binding=0) uniform sampler2D baseTexture;
#endif
#ifdef HAS_OPACITYTEXTURE
layout (set=0, binding=2) uniform sampler2D opacityTexture;
#endif
#ifdef HAS_SPECULARTEXTURE
layout (set=0, binding=3) uniform sampler2D specularTexture;
#endif

layout(set = 0, binding = 1) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	vec3 camPos;
} ubo;
layout(set = 0, binding = 2) uniform LightUniform {
	vec3 lightPos;
	vec3 lightColor;
} lightUniform;



layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {

	vec4 texColor = vec4(1.0);
	vec3 ambient = vec3(0.1);
	vec4 opacity = vec4(1.0);
	vec3 spec = vec3(0.2);
	#ifdef HAS_BASETEXTURE
	texColor = texture(baseTexture, fragTexCoord);
	#endif
	#ifdef HAS_OPACITYTEXTURE
	opacity = texture(opacityTexture, fragTexCoord);
	#endif
	#ifdef HAS_SPECULARTEXTURE
	spec = texture(specularTexture, fragTexCoord).rgb;
	#endif
	

    //vec3 lightPos = vec3(ubo.camPos.x,ubo.camPos.y,ubo.camPos.z);//TODO: Investigate the bloody mess with the coordinates flipped in Vulkan, I need to negate the light for it to get the right effect
	//lightPos.z = -lightPos.z;
	//lightPos.x = -lightPos.x;
	//lightPos.y = -lightPos.y;
	
	
	vec3 N = normalize(fragNormal);
	
	
	
	vec3 world_to_light = lightUniform.lightPos.xyz - fragPos.xyz;

	float dist = length(world_to_light);

	float atten = 1.0 / (dist *0.01);

	world_to_light = normalize(world_to_light);

	float ndotl = clamp(dot(N, world_to_light), 0.0, 1.0);


	

	
	
	
	vec3 world_to_cam = normalize(ubo.camPos-fragPos.xyz);

	vec3 R = -normalize(reflect(world_to_light, world_to_cam));
	float specular = pow(max(dot(R, world_to_cam), 0.0), 32.0);
	
    vec3 lightContribution = (ndotl * texColor.rgb + specular * spec) *atten * lightUniform.lightColor.rgb + ambient * texColor.rgb;

	outColor = vec4(lightContribution,opacity.r);
	
	
}