# Makefile for resolver application
CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = resolver
SRC = resolver.c

# Phony targets
.PHONY: all clean

# Default target
all: $(TARGET)

# Build the resolver executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up compiled files
clean:
	rm -f $(TARGET)

