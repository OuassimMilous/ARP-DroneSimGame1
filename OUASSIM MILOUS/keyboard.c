#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <ncurses.h>
#include <time.h>
#include "include/constants.h"

int main(int argc, char *argv[])
{
    // Semaphore for FIFO
    sem_t *semFIFO;
    semFIFO = sem_open(SEMFIFOPATH, O_CREAT, 0666, 1); // Initial value is 1
    if (semFIFO == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_init(semFIFO, 1, 1);
    sem_post(semFIFO);

    // Initialize ncurses
    initscr();

    // Create the FIFO if it doesn't exist
    mkfifo(FIFO_PATH, 0666);
    char myChar;

    // Set non-blocking input
    timeout(0);

    // Shared memory and semaphore for Watchdog status
    int Wshm_fd;
    int *Wsharedmem;
    sem_t *Wsem;
    Wsem = sem_open(WSEMPATH, O_CREAT, 0666, 1); // Initial value is 1
    if (Wsem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_init(Wsem, 1, 1);
    sem_post(Wsem);

    // Create a POSIX shared memory object
    Wshm_fd = shm_open(WSHMPATH, O_RDWR, 0666);
    if (Wshm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object into the program's address space
    Wsharedmem = mmap(NULL, 4 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, Wshm_fd, 0);
    if (Wsharedmem == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Semaphore for logging
    sem_t *LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); // Initial value is 1
    if (LOGsem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Modify shared memory to indicate activity
        sem_wait(Wsem);
        Wsharedmem[2] = 1; 
        sem_post(Wsem);

        int keyPressed = getch();

        if (keyPressed != ERR)
        {
            // If a key is pressed, send it through FIFO
            sem_wait(semFIFO);
            int fd = open(FIFO_PATH, O_WRONLY);
            if (fd == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            write(fd, &keyPressed, sizeof(keyPressed));
            sem_post(semFIFO);
            close(fd);

            sem_wait(LOGsem);

            // Open the file in append mode
            FILE *file = fopen(LOGPATH, "a");
            if (file == NULL)
            {
                fprintf(stderr, "Error opening the file.\n");
                exit(EXIT_FAILURE);
            }

            // Write the log message
            fprintf(file, "[Keyboard]: key %c pressed\n", keyPressed);

            // Close the file
            fclose(file);

            sem_post(LOGsem);
        }
        else
        {
            // If no key is pressed, send an empty character
            sem_wait(semFIFO);
            int fd = open(FIFO_PATH, O_WRONLY);
            if (fd == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            write(fd, &myChar, sizeof(myChar));
            sem_post(semFIFO);
            close(fd);
        }

        // Sleep for 200 milliseconds
        usleep(200000);
    }

    // End ncurses
    endwin();

    // Clean up shared memory
    munmap(Wsharedmem, 4 * sizeof(int));
    close(Wshm_fd);
    sem_close(Wsem);

    return 0;
}
