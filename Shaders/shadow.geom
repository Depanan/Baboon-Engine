#version 420

#define LIGHT_COUNT 32

layout (triangles, invocations = LIGHT_COUNT) in;
layout (triangle_strip, max_vertices = 3) out;

layout (set = 0, binding = 0) uniform UBOShadows 
{
	mat4 viewprojections[LIGHT_COUNT];
} uboShadows;

void main() 
{
	gl_Layer = gl_InvocationID;
	for (int i = 0; i < gl_in.length(); i++)
	{
		gl_Position =  uboShadows.viewprojections[gl_InvocationID] * gl_in[i].gl_Position;	
		EmitVertex();
	}
	EndPrimitive();
}