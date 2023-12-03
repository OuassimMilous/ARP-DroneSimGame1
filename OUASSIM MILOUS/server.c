#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "include/constants.h"

int main(int argc, char *argv[]) {
    // Shared memory variables
    double *sharedmem;
    int shm_fd;
    sem_t *sem;
    FILE *file;
    char slog[100];

    // Position information
    double position[6]; // Initialize positions

    // Open a POSIX shared memory object
    shm_fd = shm_open(SHMPATH, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    ftruncate(shm_fd, 6 * sizeof(double));

    // Map the shared memory object into the program's address space
    sharedmem = mmap(NULL, 6 * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedmem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Create or open a semaphore for shared memory access
    sem = sem_open(SEMPATH, O_CREAT, 0666, 1); // Initial value is 1
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Create or open a semaphore for logging
    sem_t *LOGsem = sem_open(LOGSEMPATH, O_CREAT, 0666, 1); // Initial value is 1
    if (LOGsem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Modify shared memory to indicate activity
    sem_wait(LOGsem);

    // Open the file in write mode ("w" stands for write)
    file = fopen(LOGPATH, "w");
    // Check if the file was opened successfully
    if (file == NULL) {
        fprintf(stderr, "Error opening the file.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "--------------------------------------------\n");
    // Close the file
    fclose(file);

    sem_post(LOGsem);

    // Watchdog process shared memory variables
    int Wshm_fd;
    int *Wsharedmem;
    sem_t *Wsem;

    // Create or open a semaphore for Watchdog process
    Wsem = sem_open(WSEMPATH, O_CREAT, 0666, 1); // Initial value is 1
    if (Wsem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_init(Wsem, 1, 1);
    sem_post(Wsem);

    // Create a POSIX shared memory object for Watchdog process
    Wshm_fd = shm_open(WSHMPATH, O_RDWR, 0666);
    if (Wshm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object into the program's address space for Watchdog process
    Wsharedmem = mmap(NULL, 4 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, Wshm_fd, 0);
    if (Wsharedmem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Infinite loop for printing
    while (1) {
        // Modify Watchdog shared memory to indicate activity
        sem_wait(Wsem);
        Wsharedmem[0] = 1; 
        sem_post(Wsem);

        sem_wait(LOGsem);

        file = fopen(LOGPATH, "a");
        // Check if the file was opened successfully
        if (file == NULL) {
            fprintf(stderr, "Error opening the file.\n");
            exit(EXIT_FAILURE);
        }

        sem_wait(sem); // Wait for the semaphore
        sprintf(slog, "[server] position: %lf, %lf", sharedmem[0], sharedmem[1]);
        sem_post(sem); // Release the semaphore

        // Write the string to the file
        fprintf(file, "%s\n", slog);
        // Close the file
        fclose(file);
        sem_post(LOGsem);

        usleep(100000); // Sleep for 100 milliseconds
    }

    // Cleanup
    munmap(sharedmem, 6 * sizeof(double));
    close(shm_fd);
    sem_close(sem);
    sem_unlink(SEMPATH);

    // Clean up Watchdog process shared memory
    munmap(Wsharedmem, 4 * sizeof(int));
    close(Wshm_fd);
    sem_close(Wsem);

    return 0;
}
