# Compiler
CC=clang++

# Source files
FILES=main.cpp
# Output
OUTPUT=main

# Include paths
INCLUDE=-I ../libs/quickjs -I ../libs/cppast/include/ -I ../libs/cppast/external/type_safe/external/debug_assert/ -I ../libs/cppast/external/type_safe/include/ -I ../libs/json/include/
# Linker paths
LINK=-L ~/cpp/libs/quickjs -L ../libs/cppast/build/ -L ../libs/cppast/build/src/
# Linked libraries
LIBRARIES=-lquickjs -lclang -lcppast -l_cppast_tiny_process
# Compiler options
C_OPTS=-pthread

COMMAND=$(CC) $(FILES) -o $(OUTPUT) $(INCLUDE) $(LINK) $(LIBRARIES) $(C_OPTS)

all: $(FILES)
	$(COMMAND) -O0

release:
	$(COMMAND) -O2

size:
	strip $(OUTPUT)
	upx --best $(OUTPUT)

run: all
	./$(OUTPUT)

