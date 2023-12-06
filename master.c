#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/select.h>
#include <unistd.h> 
#include <stdlib.h>
#include "include/constants.h"
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

// Global flag to indicate whether the watchdog is running
volatile sig_atomic_t watchdog_running = 1;

// Signal handler for SIGCHLD to track watchdog status
void handle_watchdog_exit(int signum) {
    watchdog_running = 0;
}

int main(int argc, char *argv[]) {

    //declaration of variables
    pid_t server, UI, drone, watchdog, keyboard;
    int res;
    int num_children = 0;

    // Creation of the a POSIX shared memory objects

    double *sharedmem;
    int shm_fd;

    // Open a POSIX shared memory object
    shm_fd = shm_open(SHMPATH, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    ftruncate(shm_fd, 6 * sizeof(double));


    //making the FIFO
    mkfifo(FIFO_PATH, 0666);

    //creation of semaphores
    sem_t *semFIFO;
    semFIFO = sem_open(SEMFIFOPATH, O_CREAT, 0666, 1); 
    if (semFIFO == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_init(semFIFO, 1, 0);


    sem_t *sem;
    sem = sem_open(SEMPATH, O_CREAT, 0666, 1); 
    if (sem == SEM_FAILED)
    {
        perror("sem_open");

        exit(EXIT_FAILURE);
    }
    sem_init(sem, 1, 0);




    sem_t *semLOG;
    semLOG = sem_open(LOGSEMPATH, O_CREAT, 0666, 1); 
    if (semLOG == SEM_FAILED)
    {
        perror("sem_open1");
        exit(EXIT_FAILURE);
    }
    sem_init(semLOG, 1, 1);

    sem_t *semcent;
    semcent = sem_open(SEMCENTPATH, O_CREAT, 0666, 1); 
    if (semcent == SEM_FAILED)
    {
        perror("sem_open1");
        exit(EXIT_FAILURE);
    }
    sem_init(semcent, 1, 0);

    //cleanup the log file
    sem_wait(semLOG);
    // Open the file in write mode ("w" stands for write)
    FILE *file = fopen(LOGPATH, "w");
    // Check if the file was opened successfully
    if (file == NULL) {
        fprintf(stderr, "Error opening the file.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "--------------------------------------------\n");
    // Close the file
    fclose(file);

    sem_post(semLOG);


    // Set up signal handler for child process exit
    signal(SIGCHLD, handle_watchdog_exit);

    // Fork server process
    server = fork();
    if (server < 0) {
        perror("Fork error at server");
        return -1;
    } else if (server == 0) {
        // Child process: Execute server
        char *arg_list[] = {"&", NULL};
        execvp("./build/server", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Fork UI process
    UI = fork();
    if (UI < 0) {
        perror("Fork error at UI");
        return -1;
    } else if (UI == 0) {
        // Child process: Execute UI
        char *arg_list[] = {"konsole", "-e", "./build/UI", NULL};
        execvp("konsole", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Fork keyboard process
    keyboard = fork();
    if (keyboard < 0) {
        perror("Fork error at keyboard");
        return -1;
    } else if (keyboard == 0) {
        // Child process: Execute keyboard
        char *arg_list[] = {"konsole", "-e", "./build/keyboard", NULL};
        execvp("konsole", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Fork drone process
    drone = fork();
    if (drone < 0) {
        perror("Fork error at drone");
        return -1;
    } else if (drone == 0) {
        // Child process: Execute drone
        char *arg_list[] = {"&", NULL};
        execvp("./build/drone", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Fork targets process (commented out)
    /*
    targets = fork();
    if (targets < 0) {
        perror("Fork error at targets");
        return -1;
    } else if(targets == 0) {
        // Child process: Execute targets
        char *arg_list[] = {"./build/targets", "&", NULL};
        execvp("konsole", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;
    */

    // Fork obstacles process (commented out)
    /*
    obstacles = fork();
    if (obstacles < 0) {
        perror("Fork error at obstacles");
        return -1;
    } else if(obstacles == 0) {
        // Child process: Execute obstacles
        char *arg_list[] = {"./build/obstacles", "&", NULL};
        execvp("konsole", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;
    */

    // Fork watchdog process
    watchdog = fork();
    if (watchdog < 0) {
        perror("Fork error at watchdog");
        return -1;
    } else if (watchdog == 0) {
        // Child process: Execute watchdog

        // Convert PIDs to strings
        char server_str[20], UI_str[20], drone_str[20], keyboard_str[20];
        snprintf(server_str, sizeof(server_str), "%d", server);
        snprintf(UI_str, sizeof(UI_str), "%d", UI);
        snprintf(drone_str, sizeof(drone_str), "%d", drone);
        snprintf(keyboard_str, sizeof(keyboard_str), "%d", keyboard);

        // Build the arg_list with PIDs as command-line arguments
        char *arg_list[] = {"konsole", "-e","./build/watchdog", server_str, UI_str, drone_str, keyboard_str, NULL};
        execvp("konsole", arg_list);
        return 0;  // Should not reach here
    }
    num_children += 1;

    // Wait for all children to terminate or for the watchdog to exit
    while (num_children > 0 && watchdog_running) {
        pid_t child_pid = wait(&res);
        if (child_pid == watchdog) {
            // Watchdog process has exited, set the flag to terminate
            watchdog_running = 0;
        }
        num_children -= 1;
    }

    // Send signals to terminate the remaining child processes
    kill(server, SIGTERM);
    kill(UI, SIGTERM);
    kill(drone, SIGTERM);
    kill(keyboard, SIGTERM);
    // Repeat for other processes

    // Wait for all remaining children to terminate
    while (num_children > 0) {
        wait(&res);
        num_children -= 1;
    }

    //clean up
    sem_unlink(SEMPATH); 
    sem_unlink(SEMFIFOPATH); 
    sem_unlink(LOGSEMPATH); 
    sem_unlink(SEMCENTPATH); 
    sem_close(sem);
    sem_close(semFIFO);
    sem_close(semLOG);
    sem_close(semcent);

    close(shm_fd);
    
    // Exit the main process
    return 0;
}

