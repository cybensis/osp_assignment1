#include <iostream>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>

// The TaskFilter needed for task 1 is different to the rest since it requires the input and output files to be used in the command.
void TaskFilter(char* wordlist, char* outputFile);
// Tasks 2 and upwards only require the wordlist as input, so I've abstracted the forking part into its own function that has the command passed in as an argument.
void TaskFilter(std::string wordlist);
void forkCommand(std::string command);

void TaskFilter(std::string wordlist) { 
    std::string command = "grep -o -w '[a-zA-Z]\\{3,15\\}' " + wordlist + " | sort -u -o ./filteredWords.txt "; 
    forkCommand(command);
    return;
}

void TaskFilter(char* wordlist, char* outputFile) {
    std::string wordlistString(wordlist);
    std::string outputFileString(outputFile);
    std::string command = "grep -o -w '[a-zA-Z]\\{3,15\\}' " + wordlistString + " | sort -u -o " + outputFileString; 
    forkCommand(command);
    return;
}

void forkCommand(std::string command) {
    std::fstream existingFileCheck;
    existingFileCheck.open("filteredWords.txt");
    if (!existingFileCheck.is_open()) {
        pid_t pid = -1;
        pid = fork();
    
        // // exit if something went wrong with the fork
        if(pid < 0) {
            std::cerr << "Error with fork, now exiting" << std::endl;
            perror("fork");
            exit(1);
        }
        // this code only gets executed by the child process
        else if(pid == 0) {
            std::cout << "Beginning filtering process" << std::endl;
            execl("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);
            std::cout << "Filtering completed" << std::endl;        }
        // the parent process executes here
        else {
            // use the "wait" function to wait for the child to complete
            wait(NULL);
        } 
    }
    else {
        std::cerr << "Filtered words file already exists, continuing anyway" << std::endl; 
    }
    return; 
}