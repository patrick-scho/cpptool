# Compiler
CC=g++

# Source files
FILES=test1.cpp
# Output
OUTPUT=main

# Include paths
INCLUDE=-I ~/cpp/libs/quickjs
# Linker paths
LINK=-L ~/cpp/libs/quickjs
# Linked libraries
LIBRARIES=-lquickjs
# Compiler options
C_OPTS=

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

