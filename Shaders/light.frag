#version 450
/* Copyright (c) 2019-2020, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
    vec4 lightPos;         // position.w represents type of light
    vec4 lightColor;            // color.w represents light intensity
   
};
layout(set = 0, binding = 4) uniform LightsInfo
{
    uint  count;
    Light lights[32];
}lightUniform;


void main()
{
   

    vec4 albedoLoad = subpassLoad(i_albedo);
	vec4 normalLoad = subpassLoad(i_normal);
	
	vec3 N = normalLoad.xyz;
	N   = normalize(2.0 * N - 1.0);
	float ambient = normalLoad.w;
	vec3 albedo = albedoLoad.xyz;
	float spec = albedoLoad.w;

	vec4  clip         = vec4(in_uv * 2.0 - 1.0, subpassLoad(i_depth).x, 1.0);
	highp vec4 world_w = ubo.inverseViewProj * clip;
    highp vec3 fragPos     = world_w.xyz/world_w.w ;
	vec3 world_to_cam = normalize(ubo.camPos-fragPos.xyz);
	
	vec3 lightContribution =ambient * albedo.rgb;
	
	for(int i = 0;i<lightUniform.count;i++)
	{
		vec3 lightPos =lightUniform.lights[i].lightPos.xyz;
		vec3 world_to_light = lightPos.xyz - fragPos.xyz;
		float dist = length(world_to_light);
		float atten = 1.0 / (dist *lightUniform.lights[i].lightColor.w);
		world_to_light = normalize(world_to_light);
		float ndotl = clamp(dot(N, world_to_light), 0.0, 1.0);
		vec3 R = normalize(reflect(-world_to_light, N));
		float specular = pow(max(dot(R, world_to_cam), 0.0), 64.0);
		lightContribution += (ndotl * albedo.rgb + specular * spec) *atten * lightUniform.lights[i].lightColor.rgb;
	}
	
	
   
    o_color = vec4(lightContribution, 1.0);
}