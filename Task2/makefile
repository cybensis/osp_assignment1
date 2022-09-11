CC = g++

CFLAGS = -Wall -Werror -std=c++1y -lpthread -g -O -lpthread

TARGET = task2

.default: all

all: task2

clean:
	rm -rf $(TARGET) *.o *.dSYM

task2: main.o
	$(CC) $(CFLAGS) -o $@ $^

task2.o: main.cpp ../TaskFilter.cpp
	$(CC) $(CFLAGS) -c main.cpp
