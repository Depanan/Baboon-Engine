#version 450

precision highp float;

layout(input_attachment_index = 0, binding = 0) uniform subpassInput i_depth;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput i_albedo;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput i_normal;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 o_color;


layout(set = 0, binding = 3) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	mat4 inverseViewProj;
	vec3 camPos;
} ubo;

struct Light
{
    vec4 lightPos;         
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
layout(set = 0, binding = 5) uniform MaterialsInfo
{
    Material materials[128];
}materialUniform;


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

void main()
{
   

    vec4 albedoLoad = subpassLoad(i_albedo);
	vec4 normalLoad = subpassLoad(i_normal);
	
	vec3 N = normalize(normalLoad.xyz);
	float matIndex = round(normalLoad.w * 128.0f);
	vec3 albedo = albedoLoad.xyz;
	float spec = albedoLoad.w;

	vec4  clip         = vec4(in_uv * 2.0 - 1.0, subpassLoad(i_depth).x, 1.0);
	highp vec4 world_w = ubo.inverseViewProj * clip;
    highp vec3 fragPos     = world_w.xyz/world_w.w ;
	vec3 world_to_cam = normalize(ubo.camPos-fragPos.xyz);
	
	
	Material theMaterial = materialUniform.materials[int(matIndex)];
	
	vec3 lightContribution =theMaterial.ambient.rgb;
	//vec3 lightContribution = vec3(0.4);
	
#ifdef DIRLIGHTS
	for(int i = 0;i<DIRLIGHTS;i++)
	{
		lightContribution += dirLight(i,int(matIndex),N, world_to_cam, albedo,spec);
	}
#endif
#ifdef SPOTLIGHTS
	for(int i = 0;i<SPOTLIGHTS;i++)
	{
		lightContribution += spotLight(i,int(matIndex),fragPos.xyz,N, world_to_cam, albedo,spec);
	}
#endif
#ifdef POINTLIGHTS
	for(int i = 0;i<POINTLIGHTS;i++)
	{
		lightContribution += pointLight(i,int(matIndex),fragPos.xyz,N, world_to_cam, albedo,spec);
	}
#endif
    o_color = vec4(lightContribution, 1.0);
}