CC = g++

CFLAGS = -Wall -Werror -std=c++1y -lpthread -g -O

TARGET = task4

.default: all

all: task4

clean:
	rm -rf $(TARGET) *.o *.dSYM

task4: main.o
	$(CC) $(CFLAGS) -o $@ $^

task4.o: main.cpp ../TaskFilter.cpp
	$(CC) $(CFLAGS) -c main.cpp
