#version 400

in vec2 texture_coordinates;
in vec4 color_pixel;

uniform sampler2D binded_texture;

out vec4 frag_colour;

void main()
{
	frag_colour = texture(binded_texture, texture_coordinates);
	frag_colour = vec4(frag_colour.rgb/frag_colour.a, frag_colour.a) * color_pixel;
}