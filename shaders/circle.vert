#version 330

layout (location = 0) in vec2 vertices;
layout (location = 1) in vec3 circle;

void
main ()
{
  gl_Position = vec4 (circle.z * vertices + circle.xy, 0.0, 1.0);
}
