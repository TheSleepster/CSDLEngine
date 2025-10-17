# -------------------------------------------------------------------------
# Makefile - unity build (default) or developer split (exe + dll)
# -------------------------------------------------------------------------

CXX    = clang++
CC     = clang

SILENT          ?=
DEBUG           ?=            # set DEBUG=1 to get debug compile flags (-g -O0)
DEVELOPER_BUILD ?=            # set DEVELOPER_BUILD=1 to produce exe + libGameRuntime.lib + dll

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
    UNAME_S := $(shell uname -s)
    DLL_EXT = .so
    LIB_EXT = .a
    OBJ_EXT = .o
    PIC_FLAG = -fPIC
	PDB_SUFFIX := $(shell head -c4 /dev/urandom | od -An -tx1 | tr -d ' \n')
	PDB_LDFLAG =

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

# Build type 
BUILD_TYPE ?= debug
ifeq ($(DEBUG),1)
    CFLAGS = -g -O0 -fno-inline-functions $(COMMON_C_FLAGS) -DINTERNAL_DEBUG=1 -DASSERT_ENABLED=1 -DDEVELOPER_BUILD=$(DEVELOPER_BUILD)
else
    CFLAGS = -O3 $(COMMON_C_FLAGS)
endif

# -------------------------------------------------------------------------
# Sources & outputs
# -------------------------------------------------------------------------
UNITY_SRC        = ../code/main.cpp
ENGINE_SPLIT_SRC = ../code/main.cpp

GAME_SRC         = ../code/g_main.cpp

# Libraries
ENGINE_LIBS = $(PLATFORM_LIBS) -L../run_tree/deps/glad/src/ -L../run_tree/deps/SDL3/lib/ -L../run_tree/deps/Freetype/ -lSDL3-Static -lglad -lfreetype
GAME_LIBS   = $(ENGINE_LIBS) 

# Outputs
ENGINE_OUT  = ../build/Thor_$(BUILD_TYPE)$(EXE_EXT)
GAME_OUT    = ../run_tree/res/game_$(BUILD_TYPE)$(DLL_EXT)
ASSET_FILE_BUILDER_SRC = ../code/asset_builder/ab_main.c
ASSET_FILE_BUILDER_OUT = ../build/asset_file_builder$(EXE_EXT)

# Tests
TESTS_SRC  = $(wildcard ../code/tests/*.c)
TESTS_OUT  = $(patsubst ../code/tests/%.c,../build/test_%.exe,$(TESTS_SRC))
TESTS_LIBS = $(PLATFORM_LIBS)

# -------------------------------------------------------------------------
# Decide engine source depending on DEVELOPER_BUILD
# -------------------------------------------------------------------------
ifeq ($(DEVELOPER_BUILD),1)
    ENGINE_SRC = $(ENGINE_SPLIT_SRC)
else
    ENGINE_SRC = $(UNITY_SRC)
endif

SHARED_FLAGS = -shared
# -------------------------------------------------------------------------
# Targets
# -------------------------------------------------------------------------
.PHONY: all engine asset_file_builder game_dll tests 

ifeq ($(DEVELOPER_BUILD),1)
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
	$(SILENT)$(CXX) $(CFLAGS) $(COMMON_INCLUDE_PATHS) $(OS_DEFINE) $< $(TESTS_LIBS) -o $@ -MMD -MF ../build/test_$(*F).d

tests: $(TESTS_OUT)

# -------------------------------------------------------------------------
# Asset file builder
# -------------------------------------------------------------------------
$(ASSET_FILE_BUILDER_OUT): $(ASSET_FILE_BUILDER_SRC) | ../build
	@echo Building Asset File Builder...
	$(SILENT)$(CXX) $(CFLAGS) $(COMMON_INCLUDE_PATHS) -Wno-undefined-internal $(OS_DEFINE) $< $(PLATFORM_LIBS) -o $@ -MMD -MF ../build/asset_file_builder.d

asset_file_builder: $(ASSET_FILE_BUILDER_OUT)

# -------------------------------------------------------------------------
# Engine EXE
# -------------------------------------------------------------------------
engine: $(ENGINE_OUT)

$(ENGINE_OUT): $(ENGINE_SRC) |  ../run_tree/res
	@echo Building Engine EXE: $@ ...
	$(SILENT)$(CXX) $(CFLAGS) $(COMMON_INCLUDE_PATHS) $(OS_DEFINE) $< $(ENGINE_LIBS) $(LDFLAGS) -o $@ -MMD -MF ../build/engine.d

# -------------------------------------------------------------------------
# Game DLL (only built when DEVELOPER_BUILD=1)
# Link the runtime static archive into the DLL so runtime is only present in the DLL.
# -------------------------------------------------------------------------
ifeq ($(DEVELOPER_BUILD),1)
game_dll: $(GAME_OUT)

$(GAME_OUT): $(GAME_SRC) | ../build ../run_tree/res
	@echo Building Game DLL: $@ with pdb suffix $(PDB_SUFFIX)...
	$(SILENT)$(DEL_PDB)
	$(SILENT)$(DEL_RDI)
	$(SILENT)$(CXX) $(CFLAGS) $(COMMON_INCLUDE_PATHS) $(OS_DEFINE) -DEVELOPER_BUILD=1 $(SHARED_FLAGS) $< $(GAME_LIBS) \
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
# Auto deps: include generated .d files when present
# -------------------------------------------------------------------------
-include ../build/engine.d
-include ../build/asset_file_builder.d
-include $(patsubst ../code/tests/%.c,../build/test_%.d,$(TESTS_SRC))

# -------------------------------------------------------------------------
# Usage:
#   make                      # unity single-TU, same as before
#   make DEVELOPER_BUILD=1    # split build: engine EXE + libGameRuntime.{a|lib} + game DLL
#   make DEBUG=1              # compile with debug flags (overrides optimisation)
#   make clean
# -------------------------------------------------------------------------
