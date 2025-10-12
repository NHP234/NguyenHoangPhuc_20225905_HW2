# Makefile for DNS Resolver Application
# Modular structure with multiple source files

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = resolver

# Source files
SRCS = main.c validation.c utils.c dns_lookup.c
OBJS = $(SRCS:.c=.o)

# Header files (for dependency tracking)
HEADERS = resolver.h validation.h utils.h dns_lookup.h

# Phony targets
.PHONY: all clean rebuild

# Default target
all: $(TARGET)

# Build the resolver executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile individual object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up compiled files
clean:
	rm -f $(TARGET) $(OBJS)

# Rebuild everything from scratch
rebuild: clean all
