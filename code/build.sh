#!/bin/bash

opts=-DASSERT_ENABLED=1
compiler_flags="-g -O0 -fno-inline-functions -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-pointer-integer-compare"
linker_flags=
include_paths="-I../code/ -I../run_tree/deps/ -I../run_tree/deps/stb/ -I../run_tree/deps/sokol/ -I../run_tree/deps/glad/include/"
libs=" -L../run_tree/deps/glad/src/ -lm -lSDL3 -lGL -ldl -lXxf86vm -lXrandr -lX11 -lXi -lXcursor -lglad"

mkdir -p ../build/
pushd ../build
rm -f *.o

clang $compiler_flags $include_paths $opts ../code/main.c $libs -o"sdl_proj_debug"

