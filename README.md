# Operating Systems Final Project

## Project Overview
This project implements a simulation of a directed weighted graph using the C programming language and the raylib graphics library.

The project is divided into three milestones:

- Milestone 1: Read graph data from a file and compute the shortest path using Dijkstra’s algorithm.
- Milestone 2: Visualize the directed weighted graph using a graphical user interface (GUI).
- Milestone 3: Simulate movement of an entity along the shortest path with interactive controls.

## Features

### Milestone 1
- Reads graph data from an input file
- Validates input data
- Implements Dijkstra’s shortest path algorithm
- Displays the shortest path and total cost

Example output:
```txt
0 -> 2 -> 5
12
```

### Milestone 2
- Displays graph nodes as circles
- Displays directed edges with arrows
- Displays edge weights
- GUI graph visualization

### Milestone 3
- Animated entity movement
- PLAY / STOP control button
- Movement based on edge weights
- 300ms movement steps
- 1 second waiting at intermediate nodes
- Arrival message at destination

### Milestone 4

- Support for multiple travelers moving simultaneously.
- Parent process creates child processes using fork().
- Each traveler is displayed with a unique color.
- Parent process manages the GUI and waits for all children to finish.

  ## Milestone 5

- Child processes independently compute their shortest paths using Dijkstra's algorithm.
- IPC implemented using pipes.
- Children send status updates to the parent process.
- Parent process receives updates and displays traveler positions in the GUI.
- Parent process prints execution logs showing traveler movement between nodes.

### IPC Mechanism
This milestone uses Unix pipes for inter-process communication (IPC).
Each child process sends messages containing traveler status information to the parent process, and the parent updates the GUI accordingly.

## Milestone 6

### Features
- Node synchronization using POSIX semaphores.
- Only one traveler may stay inside a node at any given time.
- Additional travelers wait outside the node until access is granted.
- GUI visualizes waiting travelers.
- Critical section implemented around node occupancy.

### Synchronization Mechanism
POSIX semaphores are used to protect each node.
When a traveler reaches a node, it must acquire the node semaphore before entering.
If the node is occupied, the traveler waits outside until the semaphore becomes available.
This guarantees mutual exclusion and prevents multiple travelers from occupying the same node simultaneously.

## Project Files
```txt
milestone1.c
milestone2.c
milestone3.c
milestone4.c
milestone5.c
milestone6.c
graph.txt
Makefile
README.md
```

## Compilation and Execution

### Milestone 1
Compile:
```bash
make milestone1
```

Run:
```bash
./dijkstra graph.txt
```

### Milestone 2
Compile:
```bash
make milestone2
```

Run:
```bash
./sim graph.txt
```

### Milestone 3
Compile:
```bash
make milestone3
```

Run:
```bash
./sim graph.txt
```
### Milestone 4

Compile:

```bash
make milestone4
```

Run:

```bash
./sim graph.txt
```

### Milestone 5

Compile:

```bash
make milestone5
```

Run:

```bash
./sim graph.txt
```

### Milestone 6

Compile:

```bash
make milestone6
```

Run:

```bash
./sim graph.txt
```

## Cleaning Build Files
```bash
make clean
```

## Input File Format
```txt
N M
src dst weight
...
start end
```

Example:
```txt
6 8
0 1 4
0 2 2
1 3 5
2 1 1
2 3 8
3 4 2
4 5 3
2 5 10
0 5
```

## Requirements
- GCC compiler
- raylib graphics library
- Linux / Ubuntu environment

## Team Members and Responsibilities

- Shahd Muhtaseb - Project setup and documentation
- Sara Zuheka - Dijkstra logic and GUI components
- Sarah Jweiles - Graph structures, validation, and visualization logic
- Hadeel Abbasi - Testing, Makefile updates, animation flow, and integration
