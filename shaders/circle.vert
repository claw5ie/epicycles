#version 330

layout (location = 0) in vec2 vertices;
layout (location = 1) in vec3 circle;

uniform mat4 ortho;

void
main ()
{
  gl_Position = ortho
                * vec4 (circle.z * vertices + circle.xy, 0.0, 1.0);
}
