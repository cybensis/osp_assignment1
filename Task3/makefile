CC = g++

CFLAGS = -Wall -Werror -std=c++1y -lpthread -g -O

TARGET = task3

.default: all

all: task3

clean:
	rm -rf $(TARGET) *.o *.dSYM

task3: main.o
	$(CC) $(CFLAGS) -o $@ $^

task3.o: main.cpp ../TaskFilter.cpp
	$(CC) $(CFLAGS) -c main.cpp
