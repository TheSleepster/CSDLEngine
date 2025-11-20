# -------------------------------------------------------------------------
# Makefile - unity build (default) or developer split (exe + dll)
# -------------------------------------------------------------------------
CXX    = clang++
CC     = clang

SILENT_OUTPUT ?= 

DEBUG            ?=  # set DEBUG=1 to get debug flags (-g -O0)
ASSERT_ENABLED   ?=  # set ASSERT_ENABLED=1 for runtime assertions 
INTERNAL_BUILD   ?=  # set INTERNAL_BUILD=1 for internal developer tools
PROFILER_ENABLED ?=  # set PROFILER_ENABLED=1 to enable the in-engine profiler
RENDERER_BACKEND ?=  # set the GPU rendering backend
DLL_RELOADING    ?=  # set DLL_RELOADING=1 to compile the file g_main.cpp and main.cpp as seperate builds
BUILD_TESTS      ?=  # set BUILD_TESTS=1 to compile the files in ../code/tests/
MEMORY_DEBUGGING ?=  # set MEMORY_DEBUGGING=1 to compile with memory allocation information

# -------------------------------------------------------------------------
# Platform detection
# -------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    OS_DEFINE = -DOS_WINDOWS=1
    PLATFORM_LIBS = -lopengl32 -luser32 -lgdi32 -lwinmm -lshell32 -lole32 -luuid -lversion -ladvapi32 -lsetupapi -lcfgmgr32 -loleaut32 -limm32
    LDFLAGS = -Wl,/IGNORE:4099 -Wl,/STACK:314572800 
    EXE_EXT = .exe
    DLL_EXT = .dll
    LIB_EXT = .lib
    OBJ_EXT = .obj
    DEV_NULL = NUL
    PIC_FLAG =
    PDB_SUFFIX := $(shell powershell -Command "[guid]::NewGuid().ToString('N').Substring(0,8)")
	PDB_LDFLAG = -Wl,/PDB:../build/game_$(BUILD_TYPE)_$(PDB_SUFFIX).pdb

	DEL_PDB = powershell -NoProfile -Command "Get-ChildItem ..\\build\\game_$(BUILD_TYPE)*.pdb -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue"
	DEL_RDI = powershell -NoProfile -Command "Get-ChildItem ..\\build\\game_$(BUILD_TYPE)*.rdi -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue"
else
    UNAME_S    := $(shell uname -s)
    DLL_EXT     = .so
    LIB_EXT     = .a
    OBJ_EXT     = .o
    PIC_FLAG    = -fPIC
	PDB_SUFFIX := $(shell head -c4 /dev/urandom | od -An -tx1 | tr -d ' \n')
	PDB_LDFLAG  =

	DEL_PDB = rm -f ../build/game_$(BUILD_TYPE)*.pdb || true
	DEL_RDI = rm -f ../build/game_$(BUILD_TYPE)*.rdi || true

    ifeq ($(UNAME_S),Linux)
        OS_DEFINE = -DOS_LINUX=1
        PLATFORM_LIBS = -lX11 -lGL -lm
        DEV_NULL = /dev/null
    else ifeq ($(UNAME_S),Darwin)
        OS_DEFINE = -DOS_MAC=1
        PLATFORM_LIBS = -framework Cocoa -framework OpenGL -lm
        DEV_NULL = /dev/null
    endif
endif

# -------------------------------------------------------------------------
# Common flags / includes
# -------------------------------------------------------------------------
COMMON_INCLUDE_PATHS = -I../code/ -I../run_tree/deps/ -I../run_tree/deps/stb/ -I../run_tree/deps/sokol/ -I../run_tree/deps/glad/include/ -I../run_tree/deps/SDL3/include/ -I../run_tree/deps/Freetype/include/
COMMON_C_FLAGS       = -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces -Wno-pointer-sign -Wno-incompatible-pointer-types-discards-qualifiers -Wno-null-dereference -Wno-missing-field-initializers -Wno-switch -Wno-incompatible-pointer-types -Wno-deprecated-declarations -Wno-null-pointer-subtraction -Wno-typedef-redefinition -Wno-pointer-integer-compare -Wno-writable-strings -Wno-deprecated

ENGINE_CONFIGS = -DASSERT_ENABLED=$(ASSERT_ENABLED) -DINTERNAL_BUILD=$(INTERNAL_BUILD) -DPROFILER_ENABLED=$(PROFILER_ENABLED) -DRENDERER_BACKEND=RENDERER_BACKEND_$(RENDERER_BACKEND) -DDLL_RELOADING=$(DLL_RELOADING) -DMEMORY_DEBUGGING=$(MEMORY_DEBUGGING)

# Build type 
BUILD_TYPE ?= debug
ifeq ($(DEBUG),1)
    CFLAGS = $(ENGINE_CONFIGS) -g -O0 -fno-inline-functions $(COMMON_C_FLAGS)
else
    CFLAGS = $(ENGINE_CONFIGS) -O3 $(COMMON_C_FLAGS) 
endif

# -------------------------------------------------------------------------
# Sources & outputs
# -------------------------------------------------------------------------
ENGINE_SRC             = ../code/main.cpp
GAME_SRC               = ../code/g_main.cpp
ASSET_FILE_BUILDER_SRC = ../code/asset_builder/ab_main.c

ENGINE_LIBS = $(PLATFORM_LIBS) -L../run_tree/deps/glad/src/ -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -lSDL3-Static -lglad -lfreetype
GAME_LIBS   = $(ENGINE_LIBS) 

ENGINE_OUT             = ../build/Thor_$(BUILD_TYPE)$(EXE_EXT)
GAME_OUT               = ../run_tree/res/game_$(BUILD_TYPE)$(DLL_EXT)
ASSET_FILE_BUILDER_OUT = ../build/asset_file_builder$(EXE_EXT)

SHARED_FLAGS = -shared

# -------------------------------------------------------------------------
# Targets
# -------------------------------------------------------------------------
.PHONY: all engine asset_file_builder game_dll tests 

ifeq ($(DLL_RELOADING),1)
    all: engine game_dll asset_file_builder tests
else
    all: engine asset_file_builder tests
endif

../build:
	-mkdir -p ../build

# -------------------------------------------------------------------------
# Tests pattern rule (per-test)
# -------------------------------------------------------------------------
../build/test_%.exe: ../code/tests/%.c | ../build
	@echo Building Test $(@F)... 
	$(SILENT_OUTPUT)$(CXX) $(CFLAGS) $(COMMON_INCLUDE_PATHS) $(OS_DEFINE) $< $(TESTS_LIBS) -o $@ -MMD -MF ../build/test_$(*F).d

# -------------------------------------------------------------------------
# Tests — conditional on BUILD_TESTS
# -------------------------------------------------------------------------
ifeq ($(BUILD_TESTS),1)
TESTS_SRC := $(wildcard ../code/tests/*.c)
TESTS_OUT := $(patsubst ../code/tests/%.c,../build/test_%.exe,$(TESTS_SRC))
else
TESTS_SRC :=
TESTS_OUT :=
endif

tests: $(TESTS_OUT)

# -------------------------------------------------------------------------
# Asset file builder
# -------------------------------------------------------------------------
$(ASSET_FILE_BUILDER_OUT): $(ASSET_FILE_BUILDER_SRC) | ../build
	@echo Building Asset File Builder...
	$(SILENT_OUTPUT)$(CXX) $(CFLAGS) $(COMMON_INCLUDE_PATHS) -Wno-undefined-internal $(OS_DEFINE) $< $(PLATFORM_LIBS) -o $@ -MMD -MF ../build/asset_file_builder.d

asset_file_builder: $(ASSET_FILE_BUILDER_OUT)

# -------------------------------------------------------------------------
# Engine EXE
# -------------------------------------------------------------------------
engine: $(ENGINE_OUT)

$(ENGINE_OUT): $(ENGINE_SRC) |  ../run_tree/res
	@echo Building Engine EXE: $@ ...
	$(SILENT_OUTPUT)$(CXX) -Wno-undefined-internal $(CFLAGS) $(COMMON_INCLUDE_PATHS) $(OS_DEFINE) $< $(ENGINE_LIBS) $(LDFLAGS) -o $@ -MMD -MF ../build/engine.d

# -------------------------------------------------------------------------
# Game DLL
# -------------------------------------------------------------------------
ifeq ($(DLL_RELOADING),1)
game_dll: $(GAME_OUT)

$(GAME_OUT): $(GAME_SRC) | ../build ../run_tree/res
	@echo Building Game DLL: $@ with pdb suffix $(PDB_SUFFIX)...
	$(SILENT_OUTPUT)$(DEL_PDB)
	$(SILENT_OUTPUT)$(DEL_RDI)
	$(SILENT_OUTPUT)$(CXX) $(CFLAGS) $(COMMON_INCLUDE_PATHS) $(OS_DEFINE) -DEVELOPER_BUILD=1 $(SHARED_FLAGS) $< $(GAME_LIBS) \
		$(LDFLAGS) $(PIC_FLAG) $(PDB_LDFLAG) -o $@

endif

# -------------------------------------------------------------------------
# Helpers
# -------------------------------------------------------------------------
run_asset_file_builder: asset_file_builder
	@echo Running Asset File Builder...
	../build/asset_file_builder$(EXE_EXT)

clean:
	-rm -rf ../build

# -------------------------------------------------------------------------
# Auto deps
# -------------------------------------------------------------------------

ifeq ($(BUILD_TESTS),1)
-include $(patsubst ../code/tests/%.c,../build/test_%.d,$(TESTS_SRC))
endif

# -------------------------------------------------------------------------
# Usage:
#   make                    # unity single-TU
#   make DLL_RELOADING=1    # split build: engine EXE + game DLL
#   make DEBUG=1            # compile with debug flags (overrides optimisation)
#   make clean
# -------------------------------------------------------------------------
