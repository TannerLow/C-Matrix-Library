# GNU Make docs : https://www.gnu.org/software/make/manual/make.html
# Quick tutorial: https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

PYTHON    := python
CC        := gcc
LD        := gcc
AR        := ar
override CFLAGS := $(sort -Wall -Wextra $(CFLAGS))

MODULES        := . device matrix
TEST_MODULES   := .
SRC_DIR        := $(addprefix src/,$(MODULES))
TEST_SRC_DIR   := $(addprefix test/,$(TEST_MODULES))
BUILD_DIR      := $(addprefix build/,$(SRC_DIR))
TEST_BUILD_DIR := $(addprefix build/,$(TEST_SRC_DIR))

SRC           := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
TEST_SRC      := $(foreach sdir,$(TEST_SRC_DIR),$(wildcard $(sdir)/*.c))
SRC_OBJ       := $(patsubst src/%.c,build/src/%.o,$(SRC))
TEST_OBJ      := $(patsubst test/%.c,build/test/%.o,$(TEST_SRC))
INCLUDES      := -I "./OpenCL/Nvidia" -I "./include"
TEST_INCLUDES := -I "./test"

LIBDIR := -L "./OpenCL"
LIBS := -l "x86_64/OpenCL_nvidia"


# Platform specific variables
DELETE      := 
RMDIR       := 
EXE         := 
HIDE_OUTPUT := 
PS          := 
ifeq ($(OS), Windows_NT)
	DELETE      := del /f
	RMDIR       := rmdir /s /q
	EXE         := main.exe
	HIDE_OUTPUT := 2> nul
	PS          :=\\

else
	DELETE      := rm -f
	RMDIR       := rm -rf
	EXE         := main.out
	HIDE_OUTPUT := > /dev/null
	PS          :=/
endif


.PHONY: release checkdirs clean

SHARED_DEPS = checkdirs lib/CMatrixLib.lib build/main.exe

all: checkdirs lib/CMatrixLib.lib build/main.exe
	$(PYTHON) ./line_counter.py

release:
	$(MAKE) all CFLAGS="-DNDEBUG $(CFLAGS)"

debug:
	$(MAKE) all CFLAGS="-DDEBUG $(CFLAGS)"

build/main.exe: $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(TEST_INCLUDES) $(INCLUDES) $(LIBDIR) -L "./lib" -l "CMatrixLib" $(LIBS)

lib/CMatrixLib.lib: $(SRC_OBJ)
	$(AR) rcs $@ $(foreach obj,$^, -o $(obj)) 

checkdirs: $(BUILD_DIR) $(TEST_BUILD_DIR)

$(BUILD_DIR):
ifeq ($(OS), Windows_NT)
	@IF not exist "$@" (mkdir "$@")
	@IF not exist lib (mkdir lib)
else
	@mkdir -p $@
	@mkdir -p lib
endif

$(TEST_BUILD_DIR):
ifeq ($(OS), Windows_NT)
	@IF not exist "$@" (mkdir "$@")
	@IF not exist lib (mkdir lib)
else
	@mkdir -p $@
	@mkdir -p lib
endif

clean:
	$(RMDIR) build
	$(RMDIR) lib

vpath %.c $(TEST_SRC_DIR)

define make-test-obj
$1/%.o: %.c
	$(CC) $(CFLAGS) -c $$< -o $$@ $(TEST_INCLUDES) $(INCLUDES) $(LIBDIR) -L "./lib" -l "CMatrixLib" $(LIBS)
endef

vpath %.c $(SRC_DIR)

define make-obj
$1/%.o: %.c
	$(CC) $(CFLAGS) -c $$< -o $$@ $(INCLUDES) $(LIBDIR) $(LIBS)
endef

$(foreach bdir,$(BUILD_DIR),$(eval $(call make-obj,$(bdir))))

$(foreach bdir,$(TEST_BUILD_DIR),$(eval $(call make-test-obj,$(bdir))))
