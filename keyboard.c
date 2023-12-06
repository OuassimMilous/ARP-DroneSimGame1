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
    // Initialize ncurses
    initscr();

    //declaration of variables
    char myChar;
    int keyPressed;

    // Open the semaphore for FIFO
    sem_t* semFIFO = sem_open(SEMFIFOPATH, O_RDWR, 0);
    if (semFIFO == SEM_FAILED) {
        perror("sem_open3");
        exit(EXIT_FAILURE);
    }
    sem_post(semFIFO);

    // Semaphore for logging
    sem_t *LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); // Initial value is 1
    if (LOGsem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_post(LOGsem);

 
    // Set non-blocking input
    timeout(0);
    
    while (1)
    {

        keyPressed = getch();

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
            close(fd);
            sem_post(semFIFO);

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


    return 0;
}
