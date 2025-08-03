@echo off

@echo Building Engine...
set opts=-DOS_WINDOWS=1 -DASSERT_ENABLED=1
set compiler_flags=-g -O0 -fno-inline-functions -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction 
set include_paths=-I../code/ -I../run_tree/deps/ -I../run_tree/deps/stb/ -I../run_tree/deps/sokol/ -I../run_tree/deps/glad/include/ -I../run_tree/deps/SDL3/include/ -I../run_tree/deps/Freetype/include/
set libs=-L../run_tree/deps/glad/src/ -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -lopengl32 -lSDL3-Static -luser32 -lgdi32 -lwinmm -lshell32 -lole32 -luuid -lversion -lglad -ladvapi32 -lsetupapi -lcfgmgr32 -loleaut32 -limm32 -lfreetype

IF NOT EXIST "../build" mkdir "../build"
pushd "../build/"

clang %compiler_flags% %include_paths% %opts% ../code/main.c %libs% -o"../build/sdl_proj_debug.exe" 
popd

@echo Finished...
