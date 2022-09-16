#version 330

layout (location = 0) out vec4 output_color;

uniform sampler2D sampler;

in vec2 out_tex;

void
main ()
{
  output_color = texture (sampler, out_tex);
}
