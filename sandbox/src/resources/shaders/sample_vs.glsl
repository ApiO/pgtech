#version 400

layout (location = 0) in vec3 vp;
layout (location = 1) in vec2 tp;
layout (location = 2) in vec4 cp;

uniform mat4 proj_view, model;

out vec2 texture_coordinates;
out vec4 color_pixel;

void main()
{
	texture_coordinates = tp;
	color_pixel    			= cp;
	
	gl_Position = proj_view * model * vec4 (vp, 1.0);
}