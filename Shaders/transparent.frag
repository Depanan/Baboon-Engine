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

struct Light
{
    vec4 lightPos;         // position.w represents type of light
    vec4 lightColor;            // color.w represents light intensity
   
};
layout(set = 0, binding = 4) uniform LightsInfo
{
    Light pointLights[15];
	Light spotLights[15];
	Light dirLights[2];
}lightUniform;

struct Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

layout(set = 0, binding = 6) uniform MaterialsInfo
{
    Material materials[128];
}materialUniform;


layout (push_constant) uniform PushConstantMaterialId {
	layout(offset = 64) vec4 info;
} pushConstantMaterialId;


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




vec3 pointLight(uint lightIndex,uint matIndex, vec3 position, vec3 N, vec3 world_to_cam,vec3 albedo,float spec)
{
	Material theMaterial = materialUniform.materials[int(matIndex)];
	vec3 lightPos =lightUniform.pointLights[lightIndex].lightPos.xyz;
	vec3 world_to_light = lightPos.xyz - position;
	float dist = length(world_to_light);
	float atten = 1.0 / (dist *lightUniform.pointLights[lightIndex].lightColor.w);
	world_to_light = normalize(world_to_light);
	float ndotl = clamp(dot(N, world_to_light), 0.0, 1.0);
	vec3 R = normalize(reflect(-world_to_light, N));
	float specular = pow(max(dot(R, world_to_cam), 0.0), theMaterial.specular.a);
	return max(vec3(0.0),(ndotl * albedo.rgb* theMaterial.diffuse.rgb + specular * spec* theMaterial.specular.rgb) *atten * lightUniform.pointLights[lightIndex].lightColor.rgb);
}
vec3 spotLight(uint lightIndex,uint matIndex, vec3 position, vec3 N, vec3 world_to_cam, vec3 albedo,float spec)
{
	return vec3(0.0);

}
vec3 dirLight(uint lightIndex,uint matIndex, vec3 N, vec3 world_to_cam,vec3 albedo,float spec)
{
	Material theMaterial = materialUniform.materials[int(matIndex)];
    vec3 world_to_light =normalize(-lightUniform.dirLights[lightIndex].lightPos.xyz);
	float ndotl = clamp(dot(N, -world_to_light), 0.0, 1.0);	
	
	vec3 R = normalize(reflect(world_to_light, N));
	float specular = pow(max(dot(R, world_to_cam), 0.0), theMaterial.specular.a);

	return max(vec3(0.0),(ndotl * albedo.rgb * theMaterial.diffuse.rgb+ specular * spec * theMaterial.specular.rgb)  * lightUniform.dirLights[lightIndex].lightColor.rgb);
	//return vec3(specular);
}



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
		
		
	vec3 world_to_cam = normalize(ubo.camPos-fragPos.xyz);
	float matIndex = pushConstantMaterialId.info.x;
	Material theMaterial = materialUniform.materials[int(matIndex)];
	vec3 lightContribution =theMaterial.ambient.rgb;

	
	#ifdef DIRLIGHTS
		for(int i = 0;i<DIRLIGHTS;i++)
		{
			lightContribution += dirLight(i,int(matIndex),N, world_to_cam, albedo.rgb,spec.r);
		}
	#endif
	#ifdef SPOTLIGHTS
		for(int i = 0;i<SPOTLIGHTS;i++)
		{
			lightContribution += spotLight(i,int(matIndex),fragPos.xyz,N, world_to_cam, albedo.rgb,spec.r);
		}
	#endif
	#ifdef POINTLIGHTS
		for(int i = 0;i<POINTLIGHTS;i++)
		{
			lightContribution += pointLight(i,int(matIndex),fragPos.xyz,N, world_to_cam, albedo.rgb,spec.r);
		}
	#endif
    outColor = vec4(lightContribution, opacity.r);
		
#elif defined HAS_INCOLOR
	outColor = vec4(fragColor,0.2);
	
#else
	outColor = vec4(1.0);
#endif
	
	
}