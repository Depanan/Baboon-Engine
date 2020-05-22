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
#ifdef HAS_NORMALTEXTURE
layout (set=0, binding=5) uniform sampler2D normalTexture;
#endif

layout(set = 0, binding = 1) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	mat4 inverseViewProj;
	vec3 camPos;
} ubo;

layout(set = 0, binding = 4) uniform LightUniform {
	vec3 lightPos;
	vec3 lightColor;
} lightUniform;



layout(location = 3) in vec3 fragPos;
#ifdef HAS_INCOLOR
layout(location = 0) in vec3 fragColor;
#endif
#ifdef HAS_INTEXCOORD
layout(location = 1) in vec2 fragTexCoord;
#endif
#ifdef HAS_INNORMAL
layout(location = 2) in vec3 fragNormal;
#endif

#ifdef HAS_INTANGENT
layout (location = 4) in mat3 TBN;
#endif

layout(location = 0) out vec4 outColor;

void main() {

	#ifdef HAS_INNORMAL
		vec4 albedo = vec4(1.0);
		vec3 ambient = vec3(0.1);
		vec4 opacity = vec4(1.0);
		vec3 spec = vec3(0.1);
		
		vec3 N = normalize(fragNormal);
		
		#ifdef HAS_INTEXCOORD
			#ifdef HAS_BASETEXTURE
			albedo = texture(baseTexture, fragTexCoord);
			#endif
			#ifdef HAS_OPACITYTEXTURE
			opacity = texture(opacityTexture, fragTexCoord);
			#endif
			#ifdef HAS_SPECULARTEXTURE
			spec = texture(specularTexture, fragTexCoord).rgb;
			#endif
			#ifdef HAS_NORMALTEXTURE
				#ifdef HAS_INTANGENT
				vec3 normalFromMap = 2.0 * texture(normalTexture, fragTexCoord).rgb -1.0;
				N = normalize(TBN * normalFromMap);
				#endif
			#endif
		#endif
		
		
		vec3 lightPos = vec3(ubo.camPos.x,ubo.camPos.y,ubo.camPos.z);//TODO: Investigate the bloody mess with the coordinates flipped in Vulkan, I need to negate the light for it to get the right effect
		vec3 world_to_light = lightPos.xyz - fragPos.xyz;
		float dist = length(world_to_light);
		float atten = 1.0 / (dist *0.01);
		world_to_light = normalize(world_to_light);
		float ndotl = clamp(dot(N, world_to_light), 0.0, 1.0);
		vec3 world_to_cam = normalize(ubo.camPos-fragPos.xyz);
		vec3 R = normalize(reflect(-world_to_light, N));
		float specular = pow(max(dot(R, world_to_cam), 0.0), 64.0);
		vec3 lightContribution = (ndotl * albedo.rgb + specular * spec) *atten * lightUniform.lightColor + ambient * albedo.rgb;

		

		outColor = vec4(lightContribution,opacity.r);
		
	#elif defined HAS_INCOLOR
	outColor = vec4(fragColor,0.2);
	
	#else
	outColor = vec4(1.0);
	
	#endif
	
	
}