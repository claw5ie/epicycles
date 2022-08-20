#!/bin/bash

set -xeu

files="src/main.c src/Utils.c"

cc -Wall -Wextra -pedantic -g ${files} -lglfw -lGL -lGLEW -lm
