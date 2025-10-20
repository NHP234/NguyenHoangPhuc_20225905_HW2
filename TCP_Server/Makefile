CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = server
OBJS = server.o logger.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

server.o: server.c logger.h
	$(CC) $(CFLAGS) -c server.c

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) -c logger.c

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

