CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lm

# For Windows with MinGW
ifeq ($(OS),Windows_NT)
    LDFLAGS = -lgdi32 -lopengl32 -lm
endif

# Source files
SRC_FILES = main.c lexer.c parser.c ast.c semantic.c ir.c eval.c vm.c runtime.c \
           stack.c symbol.c \
           stdlib.c class.c network.c event.c timer.c http.c widget.c gui.c \
           graphics.c method.c instance.c module.c optimize.c concurrency.c \
           opengl.c vulkan.c

# Object files
OBJ_FILES = $(SRC_FILES:.c=.o)

# Targets
OUROBOROS = ouroc.exe

all: $(OUROBOROS)

# Main Ouroboros compiler/interpreter
$(OUROBOROS): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q *.o $(OUROBOROS) 2>nul || true

run: $(OUROBOROS)
	./$(OUROBOROS)

test: $(OUROBOROS)
	./$(OUROBOROS) simple_test.ouro

.PHONY: all clean run test 
