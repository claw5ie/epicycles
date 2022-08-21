#version 330

layout (location = 0) in vec4 vertex;

out vec2 out_tex;

void
main ()
{
  out_tex = vertex.zw;
  gl_Position = vec4 (vertex.xy, 0.0, 1.0);
}
