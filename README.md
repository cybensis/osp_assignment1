# How To use
In order to compile each task with the make file on the RMIT servers, first `scl enable devtoolset-9 bash` must be ran before running `make`. 
After this simply typing `make` in each tasks subfolder should compile each task as an executable with the name of `task + task number`, e.g. `task1`.
All tasks can run without issue on the RMIT servers except task 4, this is due to the method of scheduling I've used that require the nice values of 
each thread to be set at a range from -20 to 19, but on the RMIT servers, the nice values can only be set from 15-19 which is not enough to make a difference.
For task 4 to work it is advised to:
 1. Run the task either via WSL on Windows, or on another Linux machine/VM. Note: This will not work on Mac, for whatever reason the nice values can be set 
    on mac but they don't actually do anything.
 2. The program MUST be ran with sudo or must be ran as the root user as escalated privileges are needed to set nice values.