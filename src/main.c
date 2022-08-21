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

#define FOURIER_DEGREE 16

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

typedef struct MouseButtonContext MouseButtonContext;

struct MouseButtonContext
{
  gluint buffer;
  float *points;
  size_t *point_count;
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
      if (*cont.point_count >= MAX_POINTS_COUNT)
        return;

      double xpos, ypos;
      glfwGetCursorPos (win, &xpos, &ypos);

      xpos = xpos / SCREEN_WIDTH * 2 - 1;
      ypos = -ypos / SCREEN_HEIGHT * 2 + 1;

      size_t index = 3 * *cont.point_count;

      cont.points[index + 0] = xpos;
      cont.points[index + 1] = ypos;
      cont.points[index + 2] = 0.01;

      glBindBuffer (GL_ARRAY_BUFFER, cont.buffer);
      glBufferSubData (GL_ARRAY_BUFFER,
                       index * sizeof (float),
                       3 * sizeof (float),
                       cont.points + index);

      ++*cont.point_count;
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

  gluint circle_buffer;

  {
    glCreateBuffers (1, &circle_buffer);
    glBindBuffer (GL_ARRAY_BUFFER, circle_buffer);

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

    glBufferData (GL_ARRAY_BUFFER,
                  size_in_bytes,
                  circle,
                  GL_STATIC_DRAW);

    free (circle);
  }

  gluint array;
  glCreateVertexArrays (1, &array);
  glBindVertexArray (array);

  glBindBuffer (GL_ARRAY_BUFFER, circle_buffer);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray (0);

  gluint points_buffer;
  glCreateBuffers (1, &points_buffer);
  glBindBuffer (GL_ARRAY_BUFFER, points_buffer);

  glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray (1);
  glVertexAttribDivisor (1, 1);

  size_t const size_of_points_in_bytes =
    MAX_POINTS_COUNT * 3 * sizeof (float);
  // Allocate extra point for compute_fourier_series.
  float *points = malloc_or_exit (size_of_points_in_bytes
                                  + 3 * sizeof (float));
  size_t points_count = 0;

  glBufferData (GL_ARRAY_BUFFER,
                size_of_points_in_bytes,
                NULL,
                GL_DYNAMIC_DRAW);

  gluint program;

  {
    gluint vertex_shader =
      create_shader (GL_VERTEX_SHADER, "shaders/circle.vert");
    gluint fragment_shader =
      create_shader (GL_FRAGMENT_SHADER, "shaders/circle.frag");
    program = create_program (vertex_shader, fragment_shader);

    glDeleteShader (vertex_shader);
    glDeleteShader (fragment_shader);
  }

  MouseButtonContext mouse_context = { points_buffer,
                                       points,
                                       &points_count };

  glfwSetWindowUserPointer (window, &mouse_context);

  size_t const coeff_coconut = 2 * FOURIER_DEGREE + 1;
  float *coeffs =
    malloc_or_exit (2 * coeff_coconut * sizeof (float));
  float *circles =
    malloc_or_exit (3 * (2 * FOURIER_DEGREE) * sizeof (float));

  char execute_once = 1;

  glClearColor (1.0, 0.0, 0.0, 1.0);

  while (!glfwWindowShouldClose (window))
    {
      glClear (GL_COLOR_BUFFER_BIT);

      glUseProgram (program);
      glDrawArraysInstanced (GL_TRIANGLE_FAN,
                             0,
                             CIRCLE_SAMPLES + 1,
                             points_count);

      if (points_count == 9 && execute_once)
        {
          char should_incr = 0;

          if (points_count % 2 == 0)
            {
              points[points_count + 0] =
                (points[0] + points[points_count - 2]) / 2;
              points[points_count + 1] =
                (points[1] + points[points_count - 1]) / 2;
              should_incr = 1;
            }

          compute_fourier_series (coeffs,
                                  points,
                                  points_count + should_incr,
                                  FOURIER_DEGREE);

          {
            size_t ind = 2 * FOURIER_DEGREE;
            float x = coeffs[ind + 0], y = coeffs[ind + 1];

            memmove (coeffs + 2,
                     coeffs,
                     FOURIER_DEGREE * 2 * sizeof (float));

            coeffs[0] = x;
            coeffs[1] = y;
          }

          execute_once = 0;
        }

      glfwSwapBuffers (window);

      glfwPollEvents ();
    }

  free (points);

  glfwTerminate ();

  return EXIT_SUCCESS;
}
