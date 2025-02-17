# ARP-DroneSimGame1

## Short Definitions of Active Components:

### Master (master.c)
- **Purpose:** Creates the shared memory and the semaphores, forks the processes, and launches the other active components.
- **Primitives Used:** Shared memory, semaphores, file operations, fork.

### Server (server.c)
- **Purpose:** Manages shared memory and logs drone positions.
- **Primitives Used:** Shared memory, semaphores, file operations.

### UI (ui.c)
- **Purpose:** Uses ncurses to display the drone position and UI messages.
- **Primitives Used:** Shared memory, semaphores, ncurses.

### Keyboard (keyboard.c)
- **Purpose:** Captures keyboard input and sends it to the drone through a FIFO.
- **Primitives Used:** Shared memory, semaphores, FIFO.

### Drone (drone.c)
- **Purpose:** Simulates a drone's movement based on keyboard input.
- **Primitives Used:** Shared memory, semaphores.

### Watchdog (watchdog.c)
- **Purpose:** Monitors the status of all processes and logs them.
- **Primitives Used:** Shared memory, semaphores.

## List of Components, Directories, Files:

### Directories:
- `include/`: Contains header files.
- `log/`: Contains log files.
- `/`: Contains source code files.

### Files:
- `master.c`: Main program.
- `server.c`: Server component.
- `ui.c`: UI component.
- `keyboard.c`: Keyboard component.
- `drone.c`: Drone component.
- `watchdog.c`: Watchdog component.
- `constants.h`: Header file with constant values.
- `Makefile`: Build automation file.
- `readme.md`: Documentation file.

## Instructions for Installing and Running:

### Prerequisites:
- Ensure you have the necessary libraries installed:
  - `ncurses`
  - `konsole`

### Installation:
1. Clone the repository.
2. Create the build directory:
   


