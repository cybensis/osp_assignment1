CC = g++

CFLAGS = -Wall -Werror -std=c++1y -lpthread -g -O

TARGET = task5

.default: all

all: task5

clean:
	rm -rf $(TARGET) *.o *.dSYM

task5: main.o
	$(CC) $(CFLAGS) -o $@ $^

task5.o: main.cpp ../TaskFilter.cpp
	$(CC) $(CFLAGS) -c main.cpp
