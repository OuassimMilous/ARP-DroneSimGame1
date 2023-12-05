#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "include/constants.h"
#define M 1.0
#define K 1.0
#define T 1
#define FORCEX 1.0
#define FORCEY -1.0

double forceX = 0.0, forceY = 0.0;
int exit_flag = 0;

// a functino to update the position according to euler's formula
void calc_position(char key, double position[6]) {
    switch (key) {
        case 'w':
            forceY = forceY + FORCEY;  // Move up
            break;
        case 'a':
            forceX = forceX - FORCEX;  // Move left
            break;
        case 's':
            forceY = forceY - FORCEY;  // Move down
            break;
        case 'd':
            forceX = forceX + FORCEX;  // Move right
            break;
        case 'q':
            forceX = forceX - FORCEX;  // Move left
            forceY = forceY + FORCEY;  // Move up
            break;
        case 'e':
            forceX = forceX + FORCEX;  // Move right
            forceY = forceY + FORCEY;  // Move up
            break;
        case 'z':
            forceX = forceX - FORCEX;  // Move left
            forceY = forceY - FORCEY;  // Move down
            break;
        case 'c':
            forceX = forceX + FORCEX;  // Move right
            forceY = forceY - FORCEY;  // Move down
            break;
        case 'x':
            forceX = 0;
            forceY = 0;
            break;
        case 27:
            exit_flag = 1;  // Set exit flag to true
            break;
    }

    // Calculate new positions using the euler's formula
    double newX = (forceX * T * T - M * position[4] + 2 * M * position[2] + K * T * position[0]) / (M + K * T);
    double newY = (forceY * T * T - M * position[5] + 2 * M * position[3] + K * T * position[1]) / (M + K * T);

    // Update positions in the array
    position[0] = newX;
    position[1] = newY;

    // Shift the old and older positions
    position[4] = position[2];
    position[5] = position[3];
    position[2] = position[0];
    position[3] = position[1];
}

int main(int argc, char *argv[]) {
    //declaration of variables
    char receivedChar;
    double position[6];  
    double *sharedmem;
    int shm_fd;
    sem_t *sem;
    sem_t *semFIFO;

    // Open the POSIX shared memory object
    shm_fd = shm_open(SHMPATH, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object into the program's address space
    sharedmem = mmap(NULL, 6 * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedmem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Open the semaphore
    sem = sem_open(SEMPATH, O_RDWR, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Open the semaphore for FIFO
    semFIFO = sem_open(SEMFIFOPATH, O_RDWR, 0);
    if (semFIFO == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }            
    
    sem_post(semFIFO);

    // open the semaphore and shared memory for the watchdog status 
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


    // open the semaphore for checking if the drone is centered yet
    sem_t *semcent = sem_open(SEMCENTPATH, O_RDWR, 0666);  
    if (semcent == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    //wait for the drone to be centered to start working
    sem_wait(semcent);

    //get the initial position of the drone
    sem_wait(sem);  
    memcpy(position, sharedmem, 6 * sizeof(double));
    sem_post(sem);

    while (exit_flag == 0) {
        // Modify shared memory to indicate activity
        sem_wait(Wsem);
        Wsharedmem[3] = 1; 
        sem_post(Wsem);

        sem_wait(semFIFO);
        // Open the FIFO for reading
        int fd = open(FIFO_PATH, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        // Read a character from the FIFO
        read(fd, &receivedChar, sizeof(receivedChar));
        // Display the received character
        // Close the FIFO
        close(fd);
        sem_post(semFIFO);

        //update the position
        sem_wait(sem); // Wait for the semaphore
        calc_position(receivedChar, position);
        memcpy(sharedmem, position, 6 * sizeof(double));
        sem_post(sem);
        usleep(10000); // Sleep for 100 milliseconds
    }

end:   // Cleanup
    munmap(sharedmem, 6 * sizeof(double));
    close(shm_fd);
    sem_close(sem);
    sem_close(semFIFO);

    // Clean up shared memory
    munmap(Wsharedmem, 4 * sizeof(int));
    close(Wshm_fd);
    sem_close(Wsem);

    return 0;
}
