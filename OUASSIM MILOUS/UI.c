#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <ncurses.h>
#include "include/constants.h"

int main(int argc, char const *argv[]) {
    // Initialize ncurses
    initscr();
    cbreak();  // Line buffering disabled
    keypad(stdscr, TRUE);  // Enable special keys
    curs_set(0); // Don't display cursor

    // open a semaphore for logging
    sem_t *semcent = sem_open(SEMCENTPATH, O_RDWR, 0666); 
    if (semcent == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Create or open a semaphore for logging
    sem_t *LOGsem = sem_open(LOGSEMPATH, O_RDWR, 0666); 
    if (LOGsem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_wait(LOGsem);
    // Open the log file in append mode
    FILE *file = fopen(LOGPATH, "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening the file.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "[UI]: Window created\n");
    fclose(file);
    sem_post(LOGsem);

    // Get window dimensions
    int height, width;
    getmaxyx(stdscr, height, width);

    // Set up shared memory and semaphore for drone position
    char *sharedmem;
    int shm_fd;
    sem_t *sem;

    shm_fd = shm_open(SHMPATH, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    sharedmem = mmap(NULL, 6 * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedmem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    sem = sem_open(SEMPATH, O_RDWR, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    //center the drone
    double position[6] = {(double)width / 2, (double)height / 2, (double)width / 2, (double)height / 2, (double)width / 2, (double)height / 2};

    sem_wait(sem);
    memcpy(sharedmem, position, 6 * sizeof(double));
    sem_post(sem);

    //log that we centered it
    sem_wait(LOGsem);
    file = fopen(LOGPATH, "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening the file.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "[UI]: Drone semcentralized\n");
    fclose(file);
    //allow the rest of the program to start since its centered now
    sem_post(semcent);
    sem_post(LOGsem);

    // Set up the shared memory and semaphore for Watchdog status
    int Wshm_fd;
    int *Wsharedmem;
    sem_t *Wsem;
    Wsem = sem_open(WSEMPATH, O_RDWR, 0666);
    if (Wsem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    Wshm_fd = shm_open(WSHMPATH, O_RDWR, 0666);
    if (Wshm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    Wsharedmem = mmap(NULL, 4 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, Wshm_fd, 0);
    if (Wsharedmem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Modify Watchdog shared memory to indicate activity
        sem_wait(Wsem);
        Wsharedmem[1] = 1; 
        sem_post(Wsem);

        //update the position
        sem_wait(sem);
        memcpy(position, sharedmem, 6 * sizeof(double));
        sem_post(sem);

        // Clear the screen
        clear();

        // Draw the bordered box
        box(stdscr, 0, 0);

        // Draw the item at its current position
        mvprintw((int)position[1], (int)position[0], "+");

        // Print the X coordinate
        mvprintw(1, 1, "X: %.2f", position[0]);

        // Print the Y coordinate 
        mvprintw(2, 1, "Y: %.2f", position[1]);

        // Refresh the screen
        refresh();
        usleep(100000);  // Sleep for 100 milliseconds
    }

    // End ncurses
    endwin();

    // Cleanup for shared memory and semaphores
    munmap(sharedmem, 6 * sizeof(double));
    close(shm_fd);
    sem_close(sem);

    munmap(Wsharedmem, 4 * sizeof(int));
    close(Wshm_fd);
    sem_close(Wsem);

    return 0;
}
