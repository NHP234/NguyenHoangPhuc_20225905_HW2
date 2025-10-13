# Makefile for UDP DNS Resolver Server
# Modular structure with multiple source files

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = server

# Source files
SRCS = server.c validation.c utils.c dns_lookup.c logger.c
OBJS = $(SRCS:.c=.o)

# Header files (for dependency tracking)
HEADERS = resolver.h validation.h utils.h dns_lookup.h logger.h

# Phony targets
.PHONY: all clean rebuild

# Default target
all: $(TARGET)

# Build the server executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile individual object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up compiled files
clean:
	rm -f $(TARGET) $(OBJS) log_*.txt

# Rebuild everything from scratch
rebuild: clean all

