#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Utils.h"

#define CIRCLE_SAMPLES 64
#define MAX_POINTS_COUNT 128

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
  float *points = malloc_or_exit (size_of_points_in_bytes);
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

  glClearColor (1.0, 0.0, 0.0, 1.0);

  while (!glfwWindowShouldClose (window))
    {
      glClear (GL_COLOR_BUFFER_BIT);

      glUseProgram (program);
      glDrawArraysInstanced (GL_TRIANGLE_FAN,
                             0,
                             CIRCLE_SAMPLES + 1,
                             points_count);

      glfwSwapBuffers (window);

      glfwPollEvents ();
    }

  free (points);

  glfwTerminate ();

  return EXIT_SUCCESS;
}
