CC = g++

CFLAGS = -Wall -Werror -std=c++1y -lpthread -g -O

TARGET = task1

.default: all

all: task1

clean:
	rm -rf $(TARGET) *.o *.dSYM

task1: main.o
	$(CC) $(CFLAGS) -o $@ $^

task1.o: main.cpp ../TaskFilter.cpp
	$(CC) $(CFLAGS) -c main.cpp

