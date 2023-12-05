Short Definitions of Active Components:

    Master (master.c):
        Purpose: Creates the shared memory and the semaphores and it forks the processes and launches the other active components.
        Primitives Used: Shared memory, semaphores, file operations, fork.

    Server (server.c):
        Purpose: Manages shared memory and logs drone positions.
        Primitives Used: Shared memory, semaphores, file operations.

    UI (ui.c):
        Purpose: Uses ncurses to display the drone position and UI messages.
        Primitives Used: Shared memory, semaphores, ncurses.

    Keyboard (keyboard.c):
        Purpose: Captures keyboard input and sends it to the drone through a FIFO.
        Primitives Used: Shared memory, semaphores, FIFO.

    Drone (drone.c):
        Purpose: Simulates a drone's movement based on keyboard input.
        Primitives Used: Shared memory, semaphores.

    Watchdog (watchdog.c):
        Purpose: Monitors the status of all processes and logs them.
        Primitives Used: Shared memory, semaphores.


List of Components, Directories, Files:

    Directories:
        include/: Contains header files.
        log/: Contains log files.
        /: Contains source code files.

    Files:
    	master.c: Main program.
        server.c: Server component.
        ui.c: UI component.
        keyboard.c: Keyboard component.
        drone.c: Drone component.
        watchdog.c: Watchdog component.
        constants.h: Header file with constant values.
        Makefile: Build automation file.
        readme.md: Documentation file.

Instructions for Installing and Running:

    Installation:
        Ensure you have the necessary libraries installed (ncurses).
        Compile the components using make (Execute "make" on the Ouassim Milous directory). 
        ps: if you cloned it from github you might need to execute "mkdir build" before building the project.

    Running:
        (on the Ouassim Milous directory)Execute ./build/master.

Operational Instructions:

    UI Window:
        Displays the drone's position and coordinates.
        No user interaction; visualizes the drone's movement.

    Keyboard Window:
        Press keys to control the drone's movement.
        Logs keypresses.


    Watchdog Window:
        Monitors and logs the status of all processes.\
        

Control Keys:

    W (Move Up):
        Increments the upward force on the drone, simulating an upward movement.

    A (Move Left):
        Decrements the horizontal force on the drone, simulating a leftward movement.

    S (Move Down):
        Decrements the downward force on the drone, simulating a downward movement.

    D (Move Right):
        Increments the horizontal force on the drone, simulating a rightward movement.

    Q (Move Up-Left):
        Simultaneously decrements horizontal force and increments upward force.

    E (Move Up-Right):
        Simultaneously increments horizontal force and increments upward force.

    Z (Move Down-Left):
        Simultaneously decrements horizontal force and decrements downward force.

    C (Move Down-Right):
        Simultaneously increments horizontal force and decrements downward force.

    X (Stop):
        Sets both horizontal and vertical forces to zero, stopping the drone.

    ESC (Exit):
        Exits the program.
        


