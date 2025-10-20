CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = client
OBJS = client.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

