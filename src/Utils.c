#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <GL/glew.h>

#include "Utils.h"

void *
malloc_or_exit (size_t size)
{
  void *data = malloc (size);

  if (data == NULL)
    exit (EXIT_FAILURE);

  return data;
}

void *
read_entire_file (const char *path, size_t *file_size_loc)
{
  int file_desc = open (path, O_RDONLY);

  if (file_desc == -1)
    goto fail;

  size_t file_size;

  {
    struct stat stats;

    if (fstat (file_desc, &stats) == -1)
      goto fail;

    file_size = stats.st_size;
  }

  char *file_data = malloc_or_exit (file_size + 1);

  if (read (file_desc, file_data, file_size) != (ssize_t)file_size)
    goto fail;

  file_data[file_size] = '\0';

  close (file_desc);

  if (file_size_loc != NULL)
    *file_size_loc = file_size;

  return file_data;

 fail:
  fprintf (stderr,
           "ERROR: failed to process the file \'%s\'.\n",
           path);
  exit (EXIT_FAILURE);
}

gluint
create_shader (glenum shader_type, const char *path)
{
  gluint shader = glCreateShader (shader_type);

  size_t file_size;
  char *file_data = read_entire_file (path, &file_size);

  {
    glint size = file_size;
    glShaderSource (shader, 1, (void *)&file_data, &size);
  }

  free (file_data);

  glint status;
  glCompileShader (shader);
  glGetShaderiv (shader, GL_COMPILE_STATUS, &status);

  if (status != GL_TRUE)
    {
      glint info_log_length;
      glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_log_length);

      char *error_message = malloc (info_log_length + 1);

      glGetShaderInfoLog (shader,
                          info_log_length,
                          NULL,
                          error_message);

      fprintf (stderr,
               "ERROR: failed to compile %s shader: %s",
               shader_type == GL_VERTEX_SHADER
                 ? "vertex" : "fragment",
               error_message);

      free (error_message);
      glDeleteShader (shader);
      exit (EXIT_FAILURE);
    }

  return shader;
}

gluint
create_program (gluint vertex_shader, gluint fragment_shader)
{
  gluint program = glCreateProgram ();

  glint status;
  glAttachShader (program, vertex_shader);
  glAttachShader (program, fragment_shader);
  glLinkProgram (program);
  glGetProgramiv (program, GL_LINK_STATUS, &status);

  if (status != GL_TRUE)
    {
      glint info_log_length;
      glGetProgramiv (program, GL_INFO_LOG_LENGTH, &info_log_length);

      char *error_message = malloc (info_log_length + 1);

      glGetProgramInfoLog (program,
                           info_log_length,
                           NULL,
                           error_message);

      fprintf (stderr,
               "ERROR: failed to link program: %s",
               error_message);

      free (error_message);
      glDeleteProgram (program);
      exit (EXIT_FAILURE);
    }

  glDetachShader (program, vertex_shader);
  glDetachShader (program, fragment_shader);

  return program;
}

float
rand_rangef (float min, float max)
{
  return (float)rand() / RAND_MAX * (max - min) + min;
}
