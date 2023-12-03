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
#include <curses.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>

// Global flag to indicate whether the watchdog is running
volatile sig_atomic_t watchdog_running = 1;

// Signal handler for SIGCHLD to track watchdog status
void handle_watchdog_exit(int signum) {
    watchdog_running = 0;
}

int main(int argc, char *argv[]) {
    pid_t server, UI, drone, watchdog, keyboard;
    int res;
    int num_children = 0;

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
        char *arg_list[] = {"konsole", "-e", "./build/watchdog", NULL};
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

    // Exit the main process
    return 0;
}
