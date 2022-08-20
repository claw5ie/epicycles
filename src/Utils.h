#ifndef UTILS_H
#define UTILS_H

#include "gltypes.h"

void *
malloc_or_exit (size_t size);

void *
read_entire_file (const char *path, size_t *file_size_loc);

gluint
create_shader (glenum shader_type, const char *path);

gluint
create_program (gluint vertex_shader, gluint fragment_shader);

float
rand_rangef (float min, float max);

#endif // UTILS_H
