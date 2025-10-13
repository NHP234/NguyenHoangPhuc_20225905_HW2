# Makefile for UDP DNS Resolver Client

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = client

# Source files
SRCS = client.c
OBJS = $(SRCS:.c=.o)

# Phony targets
.PHONY: all clean rebuild

# Default target
all: $(TARGET)

# Build the client executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile individual object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up compiled files
clean:
	rm -f $(TARGET) $(OBJS)

# Rebuild everything from scratch
rebuild: clean all

