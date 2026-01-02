# BASE MAKEFILE VARIABLE SETUP

## TARGET -- the name of the output binary file
## INT_DIR -- intermediate folder to use
## BIN_DIR -- where to output the binaries
## BUILD_TYPE -- the type of build (Development, Shipping) used for Intermediate folder
## 
## CFLAGS -- flags for compilation like warnings and the standard
## CXXFLAGS -- more flags for the compilation (this one is mainly used for includes)
## DEFINES -- defines for compilation
## 
## LDFLAGS -- linker flags like lib dirs
## LDLIBS -- libs for linking

OUT=$(BIN_DIR)/$(TARGET)

SRC_DIR=src
SRC_EXT=.cpp
SRC=$(shell find $(SRC_DIR) -name '*$(SRC_EXT)')

OBJ_DIR=$(INT_DIR)/obj
OBJ=$(SRC:$(SRC_DIR)/%$(SRC_EXT)=$(OBJ_DIR)/%.o)
DEP = $(OBJ:.o=.d)

.PHONY: $(TARGET)
$(TARGET): $(OUT)
