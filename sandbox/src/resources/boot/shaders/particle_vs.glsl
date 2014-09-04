#version 330
#extension GL_ARB_draw_instanced : enable

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec2 tex_coord;

uniform mat4      proj_view;
uniform sampler2D positions;
uniform sampler2D colors;
uniform vec2      data_size;

out vec2 texture_coordinates;
out vec4 pixel_color;

void main(){
	int num_cols = int(data_size.x * 4);

	float x = (gl_InstanceID*4) % num_cols;
	float y = floor((gl_InstanceID*4) / num_cols);

	mat4 m = mat4(texture2D(positions,vec2(float(x+0)/num_cols, y/data_size.y)),
	              texture2D(positions,vec2(float(x+1)/num_cols, y/data_size.y)),
	              texture2D(positions,vec2(float(x+2)/num_cols, y/data_size.y)),
	              texture2D(positions,vec2(float(x+3)/num_cols, y/data_size.y)));

	texture_coordinates = tex_coord;
	gl_Position = proj_view * m * vec4(vertex_position, 1.0);

	pixel_color = vec4(texture2D(colors,vec2(
		float(gl_InstanceID % int(data_size.x)) / data_size.x, 
		floor(gl_InstanceID / data_size.x) / data_size.y)
	));
}