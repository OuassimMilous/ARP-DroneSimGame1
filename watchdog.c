#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <ncurses.h>
#include <semaphore.h>
#include <time.h>
#include "include/constants.h"
#include <signal.h>
#include <unistd.h>

#define NUM_PROCESSES 4
#define SHARED_MEM_SIZE (NUM_PROCESSES * sizeof(int))

pid_t processesID[NUM_PROCESSES]; // Array to store process Ids
int processes[NUM_PROCESSES]; // Array to store process statuses

// a function to update the state of the processes
void update_status(){
    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        processes[i] = (kill(processesID[i], 0) == 0);
    }

}



int main(int argc, char const *argv[])
{

    //declare the variables
    FILE *file;
    char slog[100];

    // check if all arguments are recived correctly
    if (argc != (NUM_PROCESSES + 1)) {
        fprintf(stderr, "Usage: %s <arg1> <arg2> <arg3> <arg4>\n", argv[0]);
        return 1;  // Exit with an error code
    }

    // Convert command line arguments to pid_t and store in the array
    for (int i = 0; i < NUM_PROCESSES; i++) {
        processesID[i] = atoi(argv[i + 1]);
    }


    initscr(); // Initialize the curses library
    clear();   // Clear the screen


    // open the semaphore for logging
    sem_t *LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); // Initial value is 1
    if (LOGsem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    while (1)
    {
        sleep(1); //wait for a second

        //update status of processes
        update_status();

        // Display process statuses using ncurses
        mvprintw(0, 0, "1 means the process is doing computations, 0 means it is not:");
        mvprintw(1, 0, "Server: %d", processes[0]);
        mvprintw(2, 0, "UI: %d", processes[1]);
        mvprintw(3, 0, "Keyboard: %d", processes[2]);
        mvprintw(4, 0, "Drone: %d", processes[3]);
        refresh(); // Refresh the ncurses screen

        // log the data
        sem_wait(LOGsem);
        file = fopen(LOGPATH, "a");
        if (file == NULL){ fprintf(stderr, "Error opening the file.\n"); exit(EXIT_FAILURE);}
        sprintf(slog, "[watchdog] Server: %d , UI: %d, Keyboard: %d, Drone: %d", processes[0], processes[1], processes[2], processes[3]);
        fprintf(file, "%s\n", slog);
        fclose(file);
        sem_post(LOGsem);

        // Check if all processes are active (status is 1)
        for (int i = 0; i < NUM_PROCESSES; ++i)
        {
            if (processes[i] == 0)
            {
                //end the process and the program
                goto end;
            }
        }

    }

end:
    endwin(); // End curses mode

    return 0;
}
