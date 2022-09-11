
// Words are sorted into a global array then we need 13 arrays for each word length that are sorted via threads. I was thinking we get the length of the global array, divide it by 13, then each
// thread gets 1/13th to sort.

// No I think how its meant to work is we use the cleaning function which then is somehow meant to create an array from the file?? But yeah thats the global array, then we create 2 threads, map3 and reduce3, the map3 will then go through the global array and, after creating 13 arrays, it will assign each word to an array based on the length, then map3 creates 13 threads and passes in those arrays respectively and uses the qsort function, or the C++ equivalent to sort it, then outputs to a file, then after its finished each thread is meant to signal to reduce3 that its finished. reduce3 is also meant to keep track of the amount of words in each file so maybe have a value for each thread initialized to -1, then when the threads are finished set it to the number of words, then once all 13 thread numbers are >= 0, reduce3 starts sorting all of them in the same way in task2 

#include <iostream>
#include <vector>
#include "../TaskFilter.cpp"
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include<signal.h>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <iomanip>


#define EXIT_SUCCESS                0
#define WORDLIST_ARG                1
#define OUTPUT_FILE_ARG             2
#define WORD_LENGTH_ARRAY_OFFSET    3
#define ARG_COUNT                   3
#define TEMP_FILE                   "filteredWords.txt"
#define SORT_PIPE_FILE_PREPEND      "/tmp/sorted"
#define ARRAY_COUNT                 13
#define FILE_OPEN_ERROR             -1
#define THREAD_CREATE_ERROR         -1
#define PIPE_CREATE_ERROR           -1
#define MAX_RUNTIME                 10
#define MAX_WRITE_FAILS             10
#define PERCENT_100                 100
// std::endl makes the program literally take twice as long to execute so I replaced them with "\n"
#define NEWLINE                     "\n"

// Functions
void map3(char* outputFilePath);
void reduce3(char* outputFilePath, pthread_t sortThreads[ARRAY_COUNT]);
void *sortThread(void* threadArg);
void *readThread(void* threadArrg);
bool compare(const int& c, const int& d);
void cleanUp();
void signalExitAndClean(int signum);
std::string timestamp();


// Global variables
std::vector<std::string> globalVector;
pthread_mutex_t locks[ARRAY_COUNT];
auto start = std::chrono::high_resolution_clock::now();
bool earlyExit = false;

// Structures for threads
struct writeThreadData {
   std::vector<int>*  indexVector;
   std::vector<std::string>*  globalVector;
   int threadID;
};

struct readThreadData {
   std::vector<std::string>*  wordsVector;
   int threadID;
};

void signalExitAndClean(int signum){
    std::cout << timestamp() << "Program taking longer than " << MAX_RUNTIME << " second/s, cleaning up and exiting" << NEWLINE;
    earlyExit = true;
    // In the case that sort() is still running when the signal is triggered, it will cause the program to exit abnormally
    // and these temp files won't be cleaned up, so I delete them here just in case
    cleanUp();
}

void cleanUp() {
    remove(TEMP_FILE);
    for (int i = 0; i < 13; i++) {
        remove((SORT_PIPE_FILE_PREPEND + std::to_string(i)).c_str());
    }
}


int main(int argc, char *argv[]) {
    signal(SIGALRM,signalExitAndClean);
    alarm(MAX_RUNTIME);
    // In case some error prevents the signal from executing, clean up at the start of every execution
    cleanUp();
    if (argc == ARG_COUNT) {
        // First filter the file and output to a temp file called filteredWords.txt
        std::cout << timestamp() << "Beginning task filter" << NEWLINE;
        TaskFilter(argv[WORDLIST_ARG]);
        std::cout << timestamp() << "Task filter finished, starting map3" << NEWLINE;
        map3(argv[OUTPUT_FILE_ARG]);
    }
    else {
        std::cout << "Please provide a wordlist and output file as an argument, e.g." << NEWLINE;
        std::cout << "./task3 /usr/share/dict/words ./outfile.txt" << NEWLINE;
    }
    return EXIT_SUCCESS;
}



void map3(char* outputFile) {
    std::fstream filteredWords;
    filteredWords.open(TEMP_FILE,std::ios::in);
    // Wait for the TaskFilter execlp command to finish with the filteredWords.txt file
    while (!filteredWords.is_open());
    std::vector< std::vector<int> > wordVectors(ARRAY_COUNT,std::vector<int>(0));
    // Go through the entire file line by line and insert each word into the globalVector vector, then insert the corresponding index
    // value for that globalVector word, into the 2D wordVector vector based on the words size.
    if (filteredWords.is_open()) {
        int globalVectorIndex = 0;
        std::string line = "";  
        std::cout << timestamp() << "Mapping the filtered words into the global array/vector and their index values into the individual word length vectors" << NEWLINE;
        while (getline(filteredWords, line) && !earlyExit) {
            globalVector.push_back(line);
            // Using line.length() - WORD_LENGTH_ARRAY_OFFSET because I want the vector indexing to be from 0-12 not 3-15
            wordVectors[line.size() - WORD_LENGTH_ARRAY_OFFSET].push_back(globalVectorIndex);
            globalVectorIndex++;
        }
    }
    std::cout << timestamp() << "Finished mapping words to global array/vector, now creating sort threads" << NEWLINE;
    filteredWords.close();
    remove(TEMP_FILE);
    pthread_t threads[ARRAY_COUNT];
    struct writeThreadData threadData[ARRAY_COUNT];
    int i = 0;
    // Create 13 threads using a while loop, I am using a while loop here over a for loop because it's easier to integrate the graceful exit this way
    while (i < ARRAY_COUNT && !earlyExit) {
        std::cout << timestamp() << "Creating sort/write thread " << i << NEWLINE;
        pthread_mutex_init(&locks[i], NULL);
        // Locking threads here and not in sortThread because occasionally the corresponding readThread can actually acquire the lock before the sortThread 
        // can, and this causes errors as it tries to open a pipe that doesn't exist.
        pthread_mutex_lock(&locks[i]);
        // Add the correct data and references to the threadData structure 
        threadData[i].indexVector = &wordVectors[i];
        threadData[i].globalVector = &globalVector;
        threadData[i].threadID = i;
        // Keep trying to create the thread until it succeeds
        while (pthread_create(&threads[i], NULL, sortThread, (void *) &threadData[i]) == THREAD_CREATE_ERROR) {
            std::cerr << timestamp() << "Error: unable to create thread, trying again" << NEWLINE;
        }
        i++;
    }
    if (!earlyExit) { reduce3(outputFile, threads); };
    return;
}



void *sortThread(void* threadArg) {
    struct writeThreadData *threadData;
    threadData = (struct writeThreadData *) threadArg;
    std::cout << timestamp() << "Sort thread " << threadData->threadID << " successfully created" << NEWLINE;
    // Need to create the pipe before sorting because by the time reduce3 attempts to open the pipe, the sort will still be going, and the pipes won't have been created yet
    std::string pipeName = SORT_PIPE_FILE_PREPEND + std::to_string(threadData->threadID);
    std::cout << timestamp() << "Sort thread " << threadData->threadID << " creating pipe" << NEWLINE;
    if (mkfifo(pipeName.c_str(),S_IRWXU) == PIPE_CREATE_ERROR) {
        std::cerr << timestamp() << "Sort thread " << threadData->threadID << " failed to create pipe, now exiting" << NEWLINE;
        earlyExit = true; 
    }
    // std::sort using my compare function, but this passes in index values not strings, compare will use the globalArray to compare the index values as strings
    std::cout << timestamp() << "Sort thread " << threadData->threadID << " now sorting" << NEWLINE;
    std::sort(threadData->indexVector->begin(), threadData->indexVector->end(), compare);
    pthread_mutex_unlock(&locks[threadData->threadID]);
    std::cout << timestamp() << "Sort thread " << threadData->threadID << " finished sorting, now opening pipe" << NEWLINE;
    int fileDescriptor = open(pipeName.c_str(), O_WRONLY);
    int wordCount = threadData->indexVector->size();
    std::cout << timestamp() << "Sort thread " << threadData->threadID << " now writing to pipe" << NEWLINE;
    for (int i = 0; i < wordCount; i++) {
        if (earlyExit) { i = wordCount; }
        std::string writeData = threadData->globalVector->at(threadData->indexVector->at(i));
        // Since both ends of the pipe can accurately guess the size of the data being written to the pipe, based on the word length, we can use that to keep waste down to a minimum
        int bufferSize = threadData->threadID + WORD_LENGTH_ARRAY_OFFSET;
        int writeFails = 0;
        while(write(fileDescriptor, writeData.c_str(), bufferSize) == -1 && !earlyExit) {
            writeFails++;
            std::cerr << timestamp() << "Sort thread " << threadData->threadID << " failed to write " << writeFails << " time/s, trying again" << NEWLINE;
            if (writeFails == MAX_WRITE_FAILS) {
                earlyExit = true;
            }
        }
    }
    close(fileDescriptor);
    std::cout << timestamp() << "Sort thread " << threadData->threadID << " finished writing, now exiting" << NEWLINE;
    pthread_exit(NULL);
}


void *readThread(void* threadArg) {
    struct readThreadData *threadData;
    threadData = (struct readThreadData *) threadArg;
    std::cout << timestamp() << "Read thread " << threadData->threadID << " successfully created" << NEWLINE;
    // The sort thread will lock first, then will only unlock after creating the pipe and sorting the words
    pthread_mutex_lock(&locks[threadData->threadID]);
    std::cout << timestamp() << "Read thread " << threadData->threadID << " acquired lock from sort thread, now opening pipe" << NEWLINE;
    std::string pipeName = SORT_PIPE_FILE_PREPEND + std::to_string(threadData->threadID);
    int fileDescriptor = open(pipeName.c_str(), O_RDONLY);
    if (fileDescriptor == FILE_OPEN_ERROR) {
        earlyExit = true;
        std::cout << timestamp() << "Read thread " << threadData->threadID << " had an unexpected error opening the pipe, now exiting" << NEWLINE;
    }
    else {
        std::cout << timestamp() << "Read thread " << threadData->threadID << " opened pipe, now reading from it" << NEWLINE;
    }
    bool pipeEmpty = false;
    int bufferSize = threadData->threadID + WORD_LENGTH_ARRAY_OFFSET;
    while (!pipeEmpty && !earlyExit) {
        char pipeContents[bufferSize];
        // Once a read() function from a pipe returns < 1, aka its empty or closed, then we signal that the pipe is empty
        if (read(fileDescriptor, pipeContents, bufferSize) < 1) {
            pipeEmpty = true;
        }
        // If read() returns 1 or more, this means it has data, so convert the char array into a string and add it onto the vector 
        else {
            std::string tempString = "";
            for (int a = 0; a < bufferSize; a++) {
                tempString = tempString + pipeContents[a];
            }
            threadData->wordsVector->push_back(tempString);
        }
    }
    std::cout << timestamp() << "Read thread " << threadData->threadID << " finished, now deleting pipe, destroying mutex, and exiting" << NEWLINE;
    pthread_mutex_unlock(&locks[threadData->threadID]);
    pthread_mutex_destroy(&locks[threadData->threadID]);
    close(fileDescriptor);
    remove(pipeName.c_str());
    pthread_exit(NULL);
}

void reduce3(char* outputFilePath, pthread_t sortThreads[ARRAY_COUNT]) {
    std::cout << timestamp() << "Starting reduce3" << NEWLINE;
    pthread_t readThreads[ARRAY_COUNT];
    struct readThreadData threadData[ARRAY_COUNT];
    std::vector<std::vector<std::string> > wordVectors(ARRAY_COUNT,std::vector<std::string>(0));
    // Add the correct data and references to the threadData structure keep attempting to create the thread until it succeeds
    for (int i = 0; i < ARRAY_COUNT; i++) {
        std::cout << timestamp() << "Creating read thread " << i << NEWLINE;
        threadData[i].wordsVector = &wordVectors[i];
        threadData[i].threadID = i;
        while (pthread_create(&readThreads[i], NULL, readThread, (void *) &threadData[i]) == THREAD_CREATE_ERROR) {
            std::cerr << "Error: unable to create thread, trying again" << NEWLINE;
        }
    }

    for (int i = 0; i < ARRAY_COUNT; i++) {
        pthread_join(sortThreads[i], NULL);
        pthread_join(readThreads[i], NULL);
    }

    std::fstream writeFile;
    if (!earlyExit) {
        std::cout << timestamp() << "Attempting to open the output file" << NEWLINE;
        writeFile.open(outputFilePath, std::ios::out);
        if(writeFile.is_open()) {
            std::cout << timestamp() << "Output file succesfully opened, now beginning reduce process" << NEWLINE;
        }
        else {
            earlyExit = true;
            std::cerr << timestamp() << "Unexpected error opening output file" << NEWLINE;
        }
    }
    int finishedVectors = 0;
    int startingVectorIndex = 0;
    unsigned int curVectorIndex[ARRAY_COUNT] = {0};
    while (finishedVectors < ARRAY_COUNT && !earlyExit) {
        // lowestWordIndex holds the index value for the first vector index that hasn't had all its words read
        int lowestWordIndex = startingVectorIndex;
        // The lowest word substring is always initialized to the first word of the current loop, that being the word from the lowest non-empty file
        std::string lowestWordSub = wordVectors[startingVectorIndex][curVectorIndex[startingVectorIndex]].substr(2);
        // The reason for int i = startingVectorIndex+1 is because having i = startingVectorIndex will just compare the current substring against itself.
        for (int i = startingVectorIndex+1; i < ARRAY_COUNT; i++) {
            if (curVectorIndex[i] < wordVectors[i].size()) {
                std::string currentSubString = wordVectors[i][curVectorIndex[i]].substr(2);
                // Check if the currentWord substring has a lower sort order than the current lowest, if so set the lowestWordSub and lowestWordIndex to it.
                if (currentSubString < lowestWordSub) {
                    // Winner takes over the lowestWordSub and lowestWordIndex
                    lowestWordSub = currentSubString;
                    lowestWordIndex = i;
                }
            }
        }
        
        // Write the lowest word to the final file
        writeFile << wordVectors[lowestWordIndex][curVectorIndex[lowestWordIndex]] << NEWLINE;
        // Add +1 to the current index of the vector index that had the lowest sort order word
        curVectorIndex[lowestWordIndex]++;
        // If the file is at its end then signal that another file is empty.
         if (curVectorIndex[lowestWordIndex] >= wordVectors[lowestWordIndex].size()) {
            finishedVectors++;
            std::cout << timestamp() << "Words of size " << (lowestWordIndex + WORD_LENGTH_ARRAY_OFFSET) << " finished reducing, total words of this length: " << wordVectors[lowestWordIndex].size() << NEWLINE;
            // Make sure the while loop doesn't try to access an out of bounds index, i.e. 14
            if (finishedVectors != ARRAY_COUNT) {
                // While the vector corresponding to startingVectorIndex has had all of its elements read, keep incrementing it until finds one that hasn't
                while(curVectorIndex[startingVectorIndex] >= wordVectors[startingVectorIndex].size()) {
                    startingVectorIndex++;
                }
            }
        }
    }

    for (int i = 0; i < ARRAY_COUNT; i++) {
        float totalWords = globalVector.size();
        float wordsOfThisLength = wordVectors[i].size();
        std::cout << "Words of length " << i + WORD_LENGTH_ARRAY_OFFSET << " make up " << std::setprecision(2) << (wordsOfThisLength / totalWords) * PERCENT_100 <<  "% of words in the file" << NEWLINE;
    }

    std::cout << "Finished reducing, now exiting" << NEWLINE;
    // Close the user defined file
    writeFile.close();

}


bool compare(const int& c, const int& d)
{
    // First check for strings that are empty before using substr(2)
    if (globalVector[c].length() < 2 || globalVector[d].length() < 2)
        return globalVector[c].length() < globalVector[d].length();

    return globalVector[c].substr(2) < globalVector[d].substr(2);
}


std::string timestamp() {
    auto stop = std::chrono::high_resolution_clock::now();   
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::string timestamp = std::to_string(duration.count()) + "ms - ";
    return timestamp;
}