#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Utils.h"

#define CIRCLE_SAMPLES 64
#define MAX_POINTS_COUNT 128

#define FOURIER_DEGREE 2

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct MouseButtonContext MouseButtonContext;
typedef struct Arrayf Arrayf;

struct Arrayf
{
  float *data;
  size_t comps;
  size_t count;
  size_t capacity;
};

Arrayf
create_arrayf (size_t comps, size_t capacity)
{
  assert (comps > 0 && capacity > 0);

  Arrayf arr;

  arr.data = malloc_or_exit (comps * capacity * sizeof (float));
  arr.comps = comps;
  arr.count = 0;
  arr.capacity = capacity;

  return arr;
}

size_t
get_total_size_of_arrayf (const Arrayf *arr)
{
  return arr->comps * arr->capacity * sizeof (float);
}

size_t
get_size_of_arrayf (const Arrayf *arr)
{
  return arr->comps * arr->count * sizeof (float);
}

struct MouseButtonContext
{
  gluint buffer;
  Arrayf *points;
};

void
mouse_button_callback (GLFWwindow *win,
                       int button, int action, int mods)
{
  (void)mods;

  MouseButtonContext cont =
    *(MouseButtonContext *)glfwGetWindowUserPointer (win);

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
      if (cont.points->count >= MAX_POINTS_COUNT)
        return;

      double xpos, ypos;
      glfwGetCursorPos (win, &xpos, &ypos);

      xpos = xpos / SCREEN_WIDTH * 2 - 1;
      ypos = -ypos / SCREEN_HEIGHT * 2 + 1;

      size_t index = 3 * cont.points->count;

      cont.points->data[index + 0] = xpos;
      cont.points->data[index + 1] = ypos;
      cont.points->data[index + 2] = 0.01;

      glBindBuffer (GL_ARRAY_BUFFER, cont.buffer);
      glBufferSubData (GL_ARRAY_BUFFER,
                       index * sizeof (float),
                       3 * sizeof (float),
                       cont.points->data + index);

      ++cont.points->count;
    }
}

typedef struct Vec2f Vec2f;

struct Vec2f
{
  float x, y;
};

void
acc_scale_v2f (Vec2f *dst, Vec2f s, Vec2f x, Vec2f y)
{
  dst->x += s.x * x.x + s.y * y.x;
  dst->y += s.x * x.y + s.y * y.y;
}

Vec2f
integrant (float *z, float angle)
{
  float x = z[0], y = z[1];
  float c = cos (angle), s = sin (angle);

  // Compute "z * e^(-i * angle)".
  return (Vec2f){ x * c + y * s, y * c - x * s };
}

void
compute_fourier_series (float *dst, float *z, size_t count,
                        uint32_t degree)
{
  assert (count % 2 == 1);

  --count;

  float const dt = 2.0 * M_PI / count;
  float const factor = (1.0 / 3.0) / count;

  for (int32_t i = -(int32_t)degree; i <= (int32_t)degree; i++)
    {
      Vec2f coeff = { 0, 0 };

      acc_scale_v2f (&coeff,
                     (Vec2f){ factor, -factor },
                     integrant (z + 0, 0),
                     integrant (z + 3 * count, i * dt * count));

      for (size_t j = 1; j < count; j += 2)
        {
          acc_scale_v2f (&coeff,
                         (Vec2f){ 4 * factor, 2 * factor },
                         integrant (z + 3 * j, i * dt * j),
                         integrant (z + 3 * (j + 1), i * dt * (j + 1)));
        }

      size_t ind = 2 * (i + degree);
      dst[ind + 0] = coeff.x;
      dst[ind + 1] = coeff.y;
    }
}

gluint
setup_circle_samples ()
{
  size_t const size_in_bytes = (CIRCLE_SAMPLES + 1)
                               * 2 * sizeof (float);
  float *circle = malloc_or_exit (size_in_bytes);
  float *next = circle;

  next[0] = 0;
  next[1] = 0;

  for (size_t i = 0; i < CIRCLE_SAMPLES; i++)
    {
      next += 2;

      float angle = 2 * M_PI / (CIRCLE_SAMPLES - 1) * i;
      next[0] = cos (angle);
      next[1] = sin (angle);
    }

  gluint buffer;
  glCreateBuffers (1, &buffer);
  glBindBuffer (GL_ARRAY_BUFFER, buffer);
  glBufferData (GL_ARRAY_BUFFER,
                size_in_bytes,
                circle,
                GL_STATIC_DRAW);

  free (circle);

  return buffer;
}

void
create_and_attach_buffer (gluint vertex_buffer,
                          gluint *array_loc,
                          gluint *buffer_loc)
{
  gluint array, buffer;
  glCreateVertexArrays (1, &array);
  glCreateBuffers (1, &buffer);

  glBindVertexArray (array);

  glBindBuffer (GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray (0);

  glBindBuffer (GL_ARRAY_BUFFER, buffer);
  glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray (1);
  glVertexAttribDivisor (1, 1);

  *array_loc = array;
  *buffer_loc = buffer;
}

int
main (void)
{
  if (!glfwInit ())
    exit (EXIT_FAILURE);

  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);

  GLFWwindow *window = glfwCreateWindow (800,
                                         600,
                                         "Hello epicycles",
                                         NULL,
                                         NULL);

  if (!window)
    exit (EXIT_FAILURE);

  glfwMakeContextCurrent (window);

  if (glewInit () != GLEW_OK)
    exit (EXIT_FAILURE);

  glfwSetMouseButtonCallback (window, mouse_button_callback);

  gluint circle_samples_buffer = setup_circle_samples ();

  gluint points_array, points_buffer;
  create_and_attach_buffer (circle_samples_buffer,
                            &points_array,
                            &points_buffer);

  gluint circle_array, circle_buffer;
  create_and_attach_buffer (circle_samples_buffer,
                            &circle_array,
                            &circle_buffer);

  Arrayf points = create_arrayf (3, MAX_POINTS_COUNT + 1);
  Arrayf coeffs = create_arrayf (2, 2 * FOURIER_DEGREE + 1);
  Arrayf circles = create_arrayf (3, 2 * FOURIER_DEGREE + 1);

  // Remove last point for fourier series.
  --points.capacity;
  coeffs.count = coeffs.capacity;
  circles.count = circles.capacity;

  MouseButtonContext mouse_context = { points_buffer,
                                       &points };

  glfwSetWindowUserPointer (window, &mouse_context);

  glBindBuffer (GL_ARRAY_BUFFER, points_buffer);
  glBufferData (GL_ARRAY_BUFFER,
                get_total_size_of_arrayf (&points),
                NULL,
                GL_DYNAMIC_DRAW);

  glBindBuffer (GL_ARRAY_BUFFER, circle_buffer);
  glBufferData (GL_ARRAY_BUFFER,
                get_total_size_of_arrayf (&circles),
                NULL,
                GL_DYNAMIC_DRAW);

  gluint trace_array, trace_buffer;
  glCreateVertexArrays (1, &trace_array);
  glCreateBuffers (1, &trace_buffer);

  glBindBuffer (GL_ARRAY_BUFFER, trace_buffer);
  glBufferData (GL_ARRAY_BUFFER,
                2 * 2 * sizeof (float),
                NULL,
                GL_DYNAMIC_DRAW);

  glBindVertexArray (trace_array);
  glBindBuffer (GL_ARRAY_BUFFER, trace_buffer);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray (0);

  gluint connecting_lines_array;
  glCreateVertexArrays (1, &connecting_lines_array);

  glBindVertexArray (connecting_lines_array);
  glBindBuffer (GL_ARRAY_BUFFER, circle_buffer);
  glVertexAttribPointer (0,
                         2,
                         GL_FLOAT,
                         GL_FALSE,
                         3 * sizeof (float),
                         (void *)0);
  glEnableVertexAttribArray (0);

  gluint circle_program, primitive_program, texture_program;

  {
    gluint vertex_shader =
      create_shader (GL_VERTEX_SHADER, "shaders/circle.vert");
    gluint fragment_shader =
      create_shader (GL_FRAGMENT_SHADER, "shaders/circle.frag");
    circle_program = create_program (vertex_shader, fragment_shader);

    glDeleteShader (vertex_shader);
    glDeleteShader (fragment_shader);

    vertex_shader =
      create_shader (GL_VERTEX_SHADER, "shaders/primitive.vert");
    fragment_shader =
      create_shader (GL_FRAGMENT_SHADER, "shaders/primitive.frag");
    primitive_program =
      create_program (vertex_shader, fragment_shader);

    glDeleteShader (vertex_shader);
    glDeleteShader (fragment_shader);

    vertex_shader =
      create_shader (GL_VERTEX_SHADER, "shaders/texture.vert");
    fragment_shader =
      create_shader (GL_FRAGMENT_SHADER, "shaders/texture.frag");
    texture_program =
      create_program (vertex_shader, fragment_shader);

    glDeleteShader (vertex_shader);
    glDeleteShader (fragment_shader);
  }

  gluint texture_array, texture_buffer;
  glCreateVertexArrays (1, &texture_array);
  glCreateBuffers (1, &texture_buffer);

  glBindVertexArray (texture_array);
  glBindBuffer (GL_ARRAY_BUFFER, texture_buffer);
  glVertexAttribPointer (0, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray (0);

  {
    float quad[] = { -1, -1, 0, 0,
                      1, -1, 1, 0,
                     -1,  1, 0, 1,
                      1,  1, 1, 1 };

    glBindBuffer (GL_ARRAY_BUFFER, texture_buffer);
    glBufferData (GL_ARRAY_BUFFER,
                  sizeof (quad),
                  quad,
                  GL_STATIC_DRAW);
  }

  glUseProgram (texture_program);
  glUniform1i (glGetUniformLocation (texture_program, "sampler"),
               0);

  gluint trace_texture;
  glGenTextures (1, &trace_texture);
  glBindTexture (GL_TEXTURE_2D, trace_texture);
  glTexImage2D (GL_TEXTURE_2D,
                0,
                GL_RGBA,
                SCREEN_WIDTH,
                SCREEN_HEIGHT,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                NULL);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  gluint trace_framebuffer;
  glGenFramebuffers (1, &trace_framebuffer);
  glBindFramebuffer (GL_FRAMEBUFFER, trace_framebuffer);
  glFramebufferTexture2D (GL_FRAMEBUFFER,
                          GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D,
                          trace_texture,
                          0);

  if (glCheckFramebufferStatus (GL_FRAMEBUFFER)
      != GL_FRAMEBUFFER_COMPLETE)
    {
      fputs ("ERROR: framebuffer check failed.\n", stderr);
      exit (EXIT_FAILURE);
    }

  char execute_once = 1;

  float trace_line[4];
  float *prev = trace_line;
  float *curr = prev + 2;

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor (0.0, 0.0, 0.0, 0.0);
  glBindFramebuffer (GL_FRAMEBUFFER, trace_framebuffer);
  glClear (GL_COLOR_BUFFER_BIT);
  glClearColor (1.0, 0.0, 0.0, 1.0);

  float start_time = 0;

  while (!glfwWindowShouldClose (window))
    {
      float const t = glfwGetTime () - start_time;

      if (!execute_once)
        {
          circles.data[0] = coeffs.data[0];
          circles.data[1] = coeffs.data[1];

          int freq = -FOURIER_DEGREE;

          size_t i = 1;
          while (i < coeffs.count)
            {
              freq += (freq == 0);

              float c = cos (freq * t), s = sin (freq * t);
              float x = coeffs.data[2 * i + 0];
              float y = coeffs.data[2 * i + 1];

              size_t const ind = 3 * i;
              circles.data[ind - 1] = sqrt (x * x + y * y);
              circles.data[ind + 0] = x * c - y * s
                + circles.data[ind - 3];
              circles.data[ind + 1] = x * s + y * c
                + circles.data[ind - 2];

              ++freq;
              ++i;
            }

          prev[0] = curr[0];
          prev[1] = curr[1];

          --i;

          curr[0] = circles.data[3 * i + 0];
          curr[1] = circles.data[3 * i + 1];

          glBindBuffer (GL_ARRAY_BUFFER, circle_buffer);
          glBufferSubData (GL_ARRAY_BUFFER,
                           0,
                           get_size_of_arrayf (&circles),
                           circles.data);

          glBindBuffer (GL_ARRAY_BUFFER, trace_buffer);
          glBufferSubData (GL_ARRAY_BUFFER,
                           0,
                           sizeof (trace_line),
                           trace_line);
        }

      glBindFramebuffer (GL_FRAMEBUFFER, 0);
      glClear (GL_COLOR_BUFFER_BIT);

      glUseProgram (circle_program);
      glBindVertexArray (points_array);
      glDrawArraysInstanced (GL_TRIANGLE_FAN,
                             0,
                             CIRCLE_SAMPLES + 1,
                             points.count);

      if (!execute_once)
        {
          glBindVertexArray (circle_array);
          glDrawArraysInstanced (GL_LINE_LOOP,
                                 1,
                                 CIRCLE_SAMPLES,
                                 circles.count - 1);

          glUseProgram (texture_program);
          glBindVertexArray (texture_array);
          glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

          glUseProgram (primitive_program);
          glBindVertexArray (connecting_lines_array);
          glDrawArrays (GL_LINE_STRIP, 0, circles.count);

          glBindFramebuffer (GL_FRAMEBUFFER, trace_framebuffer);

          glBindVertexArray (trace_array);
          glDrawArrays (GL_LINES, 0, 2);
        }

      if (points.count == 9 && execute_once)
        {
          char should_incr = 0;

          if (points.count % 2 == 0)
            {
              points.data[points.count + 0] =
                (points.data[0] + points.data[points.count - 2]) / 2;
              points.data[points.count + 1] =
                (points.data[1] + points.data[points.count - 1]) / 2;
              should_incr = 1;
            }

          compute_fourier_series (coeffs.data,
                                  points.data,
                                  points.count + should_incr,
                                  FOURIER_DEGREE);

          {
            size_t ind = 2 * FOURIER_DEGREE;
            float x = coeffs.data[ind + 0], y = coeffs.data[ind + 1];

            memmove (coeffs.data + 2,
                     coeffs.data,
                     FOURIER_DEGREE * 2 * sizeof (float));

            coeffs.data[0] = x;
            coeffs.data[1] = y;
          }

          curr[0] = 0;
          curr[1] = 0;

          for (size_t i = 0; i < coeffs.count; i++)
            {
              curr[0] += coeffs.data[2 * i + 0];
              curr[1] += coeffs.data[2 * i + 1];
            }

          start_time = glfwGetTime ();

          execute_once = 0;
        }

      glfwSwapBuffers (window);

      glfwPollEvents ();
    }

  free (circles.data);
  free (coeffs.data);
  free (points.data);

  glfwTerminate ();

  return EXIT_SUCCESS;
}
