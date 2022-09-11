
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm> 
#include <list>
#include "../TaskFilter.cpp"
#include <unistd.h>
#include <stdio.h>
#include <chrono>
#include<signal.h>
#define EXIT_SUCCESS                0
#define WORDLIST_ARG                1
#define OUTPUT_FILE_ARG             2
#define WORD_LENGTH_ARRAY_OFFSET    3
#define ARG_COUNT                   3
#define MAX_RUNTIME                 30
#define TEMP_FILE                   "filteredWords.txt"
#define DEFAULT_REDUCE_SUBSTRING    "~"
#define ARRAY_COUNT                 13
// NEWLINE makes the program literally take twice as long to execute so I replaced them with "\n"
#define NEWLINE                     "\n"

void map2();
void sort(std::string fileName);
bool compare(const std::string& c, const std::string& d);
void reduce2(char* outputFilePath);
void cleanAndExit(int signum);
std::string timestamp();
auto start = std::chrono::high_resolution_clock::now();


void cleanAndExit(int signum){
    std::cout << "Program taking longer than " << MAX_RUNTIME << " second/s, cleaning up and exiting" << NEWLINE;
    remove(TEMP_FILE);
    for (int i = 0; i < ARRAY_COUNT; i++) {
        remove(("sorted" + std::to_string(i)).c_str());
    }
    exit(signum);
}

int main(int argc, char *argv[]) {
    if (argc == ARG_COUNT) {
        signal(SIGALRM,cleanAndExit);
        alarm(MAX_RUNTIME);
        std::cout << timestamp() << "Beginning task filter" << NEWLINE;
        TaskFilter(argv[WORDLIST_ARG]);
        std::cout << timestamp() << "Task filter finished, starting map2" << NEWLINE;
        map2();
        std::cout << timestamp() << "map2, finished beginning reduce2" << NEWLINE;
        reduce2(argv[OUTPUT_FILE_ARG]);
    }
    else {
        std::cout << "Please provide a wordlist and output file as an argument, e.g." << NEWLINE;
        std::cout << "./task2 /usr/share/dict/words ./outfile.txt" << NEWLINE;
    }
    return EXIT_SUCCESS;
}


void map2() {
    // First filter the file and output to a temp file called filteredWords.txt
    std::vector<std::vector<std::string> > wordVectors(ARRAY_COUNT);
    std::fstream wordlist;
    wordlist.open(TEMP_FILE,std::ios::in);
    // Go through the entire file line by line and insert each word into the 2D vector based on character length
   if (wordlist.is_open()){ 
      std::string line;
      while(getline(wordlist, line)){
        // std::cout << timestamp() << "Inserting word into wordVectors: " << line << NEWLINE;
        // Using line.length() - WORD_LENGTH_ARRAY_OFFSET because I want the vector indexing to be from 0-12 not 3-15
        wordVectors[line.length() - WORD_LENGTH_ARRAY_OFFSET].push_back(line);
      }
    }

    // Create the 13 word length files, add the words to it from wordVectors, sort it, then remove the files
    for (int i = 0; i < wordVectors.size(); i++) {
        std::string unsortedFileName = std::to_string(i) + ".txt";
        std::fstream unsortedFile;
        std::cout << timestamp() << "Opening file " << unsortedFileName << NEWLINE;
        unsortedFile.open(unsortedFileName,std::ios::out);            
        for (int a = 0; a < wordVectors[i].size(); a++) {
            // std::cout << timestamp() << "Adding word to file " << unsortedFileName << ": " << wordVectors[i][a] << NEWLINE;
            unsortedFile << wordVectors[i][a] << NEWLINE;
        }
        std::cout << timestamp() << "Finished adding words, now closing file " << unsortedFileName << NEWLINE;
        unsortedFile.close();
        std::cout << timestamp() << "Sorting the words in the file " << unsortedFileName << NEWLINE;
        sort(unsortedFileName);
        std::cout << timestamp() << "Removing file " << unsortedFileName << NEWLINE;
        remove(unsortedFileName.c_str());
    }
    // Done with the global words file so it can now be removed
    remove(TEMP_FILE);
    return;
}



void sort(std::string fileName) {
    pid_t pid;
    pid = fork();
    if (pid < 0) { 
        std::cerr << timestamp() << "Error with fork, now exiting" << NEWLINE;
        return;       
    } else if (pid == 0) { 
        std::cout << timestamp() << "Beginning sort command on " << fileName << NEWLINE;
        // Sort with -k 1.3 to sort on the first word "1", third character "3", then output to a file called "sorted0.txt, sorted1.txt, etc"
        execlp("/usr/bin/sort","sort","-k","1.3",fileName.c_str(),"-o", ("sorted" + fileName).c_str() ,NULL);
        std::cout << timestamp() << "Sort command finished on " << fileName << NEWLINE;
    } else { 
        wait(NULL);
    }
    return;
}



void reduce2(char* outputFilePath) {
   std::fstream files[ARRAY_COUNT];
    std::string currentWord[ARRAY_COUNT];
    // Open all 13 sorted word files and initialize the currentWord array with a word from each file
    for (int i = 0; i < ARRAY_COUNT; i++) {
        std::cout << timestamp() << "Opening file " << ("sorted" + std::to_string(i) + ".txt") << NEWLINE;
        files[i].open(("sorted" + std::to_string(i) + ".txt").c_str(), std::ios::in);
        std::cout << timestamp() << "Getting first line from this file" << NEWLINE;
        getline(files[i],currentWord[i]);
    }

    std::fstream writeFile;
    std::cout << timestamp() << "Opening the output file at " << outputFilePath << NEWLINE;
    writeFile.open(outputFilePath, std::ios::out);
    // Everytime one of the 13 files have been completely read, emptyFiles will have 1 added to it, once it reaches 13, this means there is no more
    // words to sort and the while loop ends.
    int emptyFiles = 0;
    std::cout << timestamp() << "Beginning reduce sorting loop" << NEWLINE;
    // This variable refers to the files 0-12, and holds the lowest of these files. This is used to shorten the for loop below and initialize lowestWordSub
    int lowestOpenFile = 0;
    while (emptyFiles < ARRAY_COUNT) {
        // lowestWordIndex holds the index value for the currentWord and files array, and is used to show which of the 13 has the lowest sort order
        int lowestWordIndex = lowestOpenFile;
        // the lowest word substring is always initialized to the first word of the current loop, that being the word from the lowest non-empty file
        std::string lowestWordSub = currentWord[lowestOpenFile].substr(2);
        // The reason for int i = lowestOpenFile+1 is because having i = lowestOpenFile will just compare the current substring against itself.
        for (int i = lowestOpenFile+1; i < ARRAY_COUNT; i++) {
            // if the file is not at its end, then check if the currentWord substring has a lower sort order than the current lowest, if so set the lowestWordSub and lowestWordIndex to it.
            if (!files[i].eof() && currentWord[i].substr(2) < lowestWordSub) {
                // std::cout << timestamp() <<  currentWord[a].substr(2) << " found as lower than " << lowestWordSub << ", setting it to the current lowest" << NEWLINE;
                lowestWordSub = currentWord[i].substr(2);
                lowestWordIndex = i;
            }
        }
        
        // std::cout << timestamp() << currentWord[lowestWordIndex] << " is the lowest word for this loop, writing it to the output file " << NEWLINE;
        // Write the lowest word to the final file
        writeFile << currentWord[lowestWordIndex] << NEWLINE;
        // Get the next line only for the word that was the lowest sort order
        // std::cout << timestamp() << "Getting new line from the file " << ("sorted" + std::to_string(lowestWordIndex) + ".txt") << NEWLINE;
        getline(files[lowestWordIndex],currentWord[lowestWordIndex]);
        // If the file is at its end then signal that another file is empty.
        if (files[lowestWordIndex].eof()) {
            std::cout << timestamp() << "File " << ("sorted" + std::to_string(lowestWordIndex) + ".txt") << " is now empty" <<NEWLINE;
            emptyFiles++;
            // Make sure the while loop doesn't try to access an out of bounds index, i.e. 14
            if (emptyFiles != ARRAY_COUNT) {
                // While the lowest open file is empty, keep incrementing it until finds the lowest file that isn't empty
                while(files[lowestOpenFile].eof()) {
                    lowestOpenFile++;
                }
            }
        }
    }

    // Clean everything up by closing the 13 files and deleting them
    for (int i = 0; i < ARRAY_COUNT; i++) {
        std::cout << timestamp() << "Closing file " << ("sorted" + std::to_string(i) + ".txt") <<NEWLINE;
        files[i].close();
        std::cout << timestamp() << "Removing file " << ("sorted" + std::to_string(i) + ".txt") <<NEWLINE;
        remove(("sorted" + std::to_string(i) + ".txt").c_str());
    }
    writeFile.close();
}


bool compare(const std::string& c, const std::string& d) {
    // First check for strings that are empty before using substr(2)
    if (c.length() < 2 || d.length() < 2)
        return c.length() < d.length();
    return c.substr(2) < d.substr(2);
}

std::string timestamp() {
    auto stop = std::chrono::high_resolution_clock::now();   
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::string timestamp = std::to_string(duration.count()) + "ms - ";
    return timestamp;
}