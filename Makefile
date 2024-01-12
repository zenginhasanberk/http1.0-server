# Compiler and Compiler Flags
CC=gcc
CFLAGS=-Wall -g

# Project name
PROJECT=server

# Source and Object files
SRC=main.c server.c helpers.c
OBJ=$(SRC:.c=.o)

# Target to create the executable
$(PROJECT): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Generic rule for creating object files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean target to remove object files and executable
clean:
	rm -f $(PROJECT) $(OBJ)

# Phony targets
.PHONY: clean

