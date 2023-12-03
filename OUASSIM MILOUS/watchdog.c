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

#define NUM_PROCESSES 4
#define SHARED_MEM_SIZE (NUM_PROCESSES * sizeof(int))

int main(int argc, char const *argv[])
{
    initscr(); // Initialize the curses library
    clear();   // Clear the screen

    int processes[NUM_PROCESSES]; // Array to store process statuses
    int *sharedmem;
    int shm_fd;
    sem_t *sem;

    // Create a POSIX shared memory object
    shm_fd = shm_open(WSHMPATH, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    ftruncate(shm_fd, SHARED_MEM_SIZE);

    // Map the shared memory object into the program's address space
    sharedmem = mmap(NULL, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedmem == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Create or open a semaphore
    sem = sem_open(WSEMPATH, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Create or open a semaphore for logging
    sem_t *LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); // Initial value is 1
    if (LOGsem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    FILE *file;
    char slog[100];

    while (1)
    {
        // Reset process statuses to 0
        for (int i = 0; i < NUM_PROCESSES; ++i)
        {
            processes[i] = 0;
        }

        sem_wait(sem);                // Wait for the semaphore
        memcpy(sharedmem, processes, SHARED_MEM_SIZE);
        sem_post(sem);                // Release the semaphore
        sleep(1);

        sem_wait(sem);                // Wait for the semaphore
        memcpy(processes, sharedmem, SHARED_MEM_SIZE);
        sem_post(sem);                // Release the semaphore

        // Display process statuses using ncurses
        mvprintw(0, 0, "1 means the process is doing computations, 0 means it is not:");
        mvprintw(1, 0, "Server: %d", processes[0]);
        mvprintw(2, 0, "UI: %d", processes[1]);
        mvprintw(3, 0, "Keyboard: %d", processes[2]);
        mvprintw(4, 0, "Drone: %d", processes[3]);

        // Modify shared memory to indicate activity
        sem_wait(LOGsem);

        // Open the file in write mode ("a" stands for append)
        file = fopen(LOGPATH, "a");

        // Check if the file was opened successfully
        if (file == NULL)
        {
            fprintf(stderr, "Error opening the file.\n");
            exit(EXIT_FAILURE);
        }

        // Format the log message
        sprintf(slog, "[watchdog] Server: %d , UI: %d, Keyboard: %d, Drone: %d",
                processes[0], processes[1], processes[2], processes[3]);

        // Write the log message to the file
        fprintf(file, "%s\n", slog);

        // Close the file
        fclose(file);

        sem_post(LOGsem);

        // Check if all processes are active (status is 1)
        for (int i = 0; i < NUM_PROCESSES; ++i)
        {
            if (processes[i] == 0)
            {
                goto end;
            }
        }

        refresh(); // Refresh the ncurses screen
    }

end:
    endwin(); // End curses mode

    munmap(sharedmem, SHARED_MEM_SIZE);
    close(shm_fd);
    sem_close(sem);
    sem_unlink(WSEMPATH); 

    return 0;
}
