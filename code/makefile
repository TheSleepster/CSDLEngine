CC=clang

ifeq ($(OS),Windows_NT)
    OS_DEFINE = -DOS_WINDOWS=1
    PLATFORM_LIBS = -lopengl32 -luser32 -lgdi32 -lwinmm -lshell32 -lole32 -luuid -lversion -ladvapi32 -lsetupapi -lcfgmgr32 -loleaut32 -limm32
    EXE_EXT = .exe
    DEV_NULL = NUL
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        OS_DEFINE = -DOS_LINUX=1
        PLATFORM_LIBS = -lX11 -lGL -lm
        DEV_NULL = /dev/null
    else ifeq ($(UNAME_S),Darwin)
        OS_DEFINE = -DOS_MAC=1
        DEV_NULL = /dev/null
    endif
endif

SILENT ?=

COMMON_INCLUDE_PATHS = -I../code/ -I../run_tree/deps/ -I../run_tree/deps/stb/ -I../run_tree/deps/sokol/ -I../run_tree/deps/glad/include/ -I../run_tree/deps/SDL3/include/ -I../run_tree/deps/Freetype/include/
COMMON_C_FLAGS = -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction

BUILD_TYPE ?= debug
ifeq ($(BUILD_TYPE),release)
    CFLAGS = -O2 $(COMMON_C_FLAGS)
else
    CFLAGS = -g -O0 -fno-inline-functions $(COMMON_C_FLAGS) -DASSERT_ENABLED=1
endif

ENGINE_LIBS = $(PLATFORM_LIBS) -L../run_tree/deps/glad/src/ -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -lSDL3-Static -lglad -lfreetype
ENGINE_SRC = ../code/main.c
ENGINE_OUT = ../build/Thor_$(BUILD_TYPE)$(EXE_EXT)

ASSET_FILE_BUILDER_LIBS = $(PLATFORM_LIBS)
ASSET_FILE_BUILDER_SRC = ../code/asset_builder/ab_main.c
ASSET_FILE_BUILDER_OUT = ../build/asset_file_builder$(EXE_EXT)

all: asset_file_builder engine

asset_file_builder: $(ASSET_FILE_BUILDER_OUT)

engine: $(ENGINE_OUT)

run_asset_file_builder: asset_file_builder
	@echo Running Asset File Builder...
	../build/asset_file_builder$(EXE_EXT)

$(ASSET_FILE_BUILDER_OUT): $(ASSET_FILE_BUILDER_SRC) | ../build
	$(SILENT)$(CC) $(CFLAGS) $(COMMON_INCLUDE_PATHS) $(OS_DEFINE) $< $(ASSET_FILE_BUILDER_LIBS) -o $@ -MMD -MF ../build/asset_file_builder.d

$(ENGINE_OUT): $(ENGINE_SRC) | ../build
	$(SILENT)$(CC) $(CFLAGS) $(COMMON_INCLUDE_PATHS) $(OS_DEFINE) $< $(ENGINE_LIBS) -o $@ -MMD -MF ../build/engine.d

../build:
	mkdir -p ../build

clean:
	rm -rf ../build

.PHONY: all engine asset_file_builder run_asset_file_builder clean

-include ../build/engine.d
-include ../build/asset_file_builder.d
