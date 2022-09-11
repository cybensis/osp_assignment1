
#include <iostream>
#include "../TaskFilter.cpp"
#include<signal.h>

#define EXIT_SUCCESS                0
#define WORDLIST_ARG                1
#define OUTPUT_FILE_ARG             2
#define MAX_RUNTIME                 10
#define ARG_COUNT                   3


void exitProgram(int signum);

int main(int argc, char *argv[]) {
    if (argc == ARG_COUNT) {
        signal(SIGALRM,exitProgram);
        alarm(MAX_RUNTIME);
        TaskFilter(argv[WORDLIST_ARG], argv[OUTPUT_FILE_ARG]);
        std::cout << "Program finished, now exiting" << std::endl;
    }
    else {
        std::cout << "Please provide a wordlist and output file as an argument, e.g." << std::endl;
        std::cout << "./task1 /usr/share/dict/words ./outfile.txt" << std::endl;
    }
    return EXIT_SUCCESS;
}

void exitProgram(int signum){
    std::cout << "Program taking longer than " << MAX_RUNTIME << " second/s, exiting program" << std::endl;
    exit(signum);
}