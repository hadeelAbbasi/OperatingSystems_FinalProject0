# Operating Systems Final Project

## Project Overview
This project implements a traffic simulation on a directed weighted graph using the C programming language and the raylib graphics library.

The project is developed in seven milestones. Each milestone adds a new Operating Systems concept to the same graph simulation project: graph representation, Dijkstra’s shortest path algorithm, GUI visualization, animation, processes, inter-process communication, synchronization, and scheduling.

---

## Milestones Summary

| Milestone | Topic | Main Goal |
|---|---|---|
| Milestone 1 | Graph + Dijkstra | Read a weighted directed graph from a file and calculate the shortest path. |
| Milestone 2 | GUI | Display the graph visually using raylib. |
| Milestone 3 | Animation | Move one traveler along the shortest path with play/stop controls. |
| Milestone 4 | Processes + Signals | Support multiple travelers using child processes created with `fork()`. |
| Milestone 5 | IPC | Let child processes compute and report their movement to the parent process. |
| Milestone 6 | Synchronization | Prevent more than one traveler from occupying the same node at the same time. |
| Milestone 7 | Scheduling | Choose the next traveler entering a node using scheduling algorithms. |

---

## Milestone 1 — Directed Weighted Graph and Dijkstra Algorithm

### Goal
Build the algorithmic base of the project by representing a directed weighted graph in memory, reading it from a text file, and finding the shortest path between two nodes using Dijkstra’s algorithm.

### Features
- Reads graph data from an input file.
- Represents a directed weighted graph in memory.
- Validates input data.
- Rejects invalid negative values.
- Handles disconnected graphs.
- Handles the case where the source and destination are the same.
- Implements Dijkstra’s shortest path algorithm.
- Prints the full shortest path and total cost to the terminal.

### Expected Output Example
```txt
0 -> 2 -> 5
12
```

### No Path Example
```txt
No path found
```

### Same Source and Destination Example
```txt
0
0
```

### Relevant File
```txt
milestone1.c
```

### Compile
```bash
make milestone1
```

### Run
```bash
./dijkstra graph.txt
```

---

## Milestone 2 — Static Graph Visualization with GUI

### Goal
Display the graph read from the input file using the raylib graphics library.

### Features
- Opens a graphical window.
- Draws graph nodes as circles.
- Displays each node ID.
- Draws directed edges with arrows.
- Displays edge weights.
- Places graph nodes in readable positions.
- Keeps the graph static on screen.
- Supports up to 15 nodes.

### Relevant File
```txt
milestone2.c
```

### Compile
```bash
make milestone2
```

### Run
```bash
./sim graph.txt
```

---

## Milestone 3 — Single Traveler Animation

### Goal
Add a moving traveler that travels from a source node to a destination node using the shortest path calculated by Dijkstra’s algorithm.

### Features
- Displays the graph in the background.
- Calculates the shortest path from source to destination.
- Displays a moving traveler on the graph.
- Adds a PLAY / STOP button to control the animation.
- Moves the traveler along the shortest path.
- Edge movement is based on edge weight.
- An edge with weight `W` is divided into `W` equal movement steps.
- Each movement step takes 300 milliseconds.
- The traveler waits one full second at intermediate nodes.
- The traveler does not wait at the source or destination.
- Displays an arrival message when the destination is reached.

### Relevant File
```txt
milestone3.c
```

### Compile
```bash
make milestone3
```

### Run
```bash
./sim graph.txt
```

---

## Milestone 4 — Multiple Travelers, Processes, and Signals

### Goal
Move from a single traveler to multiple travelers moving simultaneously. The parent process manages the GUI and creates child processes using `fork()`.

### Features
- Reads an extended input file that includes multiple travelers.
- Each traveler has a source node and destination node.
- The parent process reads the graph and traveler data.
- The parent process calculates the route for each traveler.
- The parent process creates a child process for each traveler using `fork()`.
- Each child process prints a started message after creation.
- The parent process manages the GUI loop.
- Each traveler is displayed with a unique color.
- Travelers move simultaneously on the screen.
- The parent process waits for all child processes before exiting.
- When a traveler finishes its route, the parent sends a signal to the matching child process.

### Relevant File
```txt
milestone4.c
```

### Compile
```bash
make milestone4
```

### Run
```bash
./sim graph.txt
```

---

## Milestone 5 — Inter-Process Communication (IPC)

### Goal
Move to a more autonomous process model. Each child process calculates its own route and reports its current movement status to the parent process.

### Features
- Child processes independently calculate their shortest path using Dijkstra’s algorithm.
- Each child process moves along its own route.
- IPC is implemented using Unix pipes.
- Children send movement updates to the parent process.
- The parent process receives child messages.
- The parent updates traveler locations in the GUI according to received messages.
- The parent prints execution logs to the terminal.
- Terminal logs are printed only by the parent process.

### IPC Mechanism
This milestone uses Unix pipes for inter-process communication.

Each child process sends structured status messages to the parent process. The message contains the traveler’s current node and next node. The parent reads these messages, updates the GUI, and prints the required movement log.

### Log Example
```txt
[PID=1021] arrived at node 0 | next node: 2
[PID=1022] arrived at node 2 | next node: 1
[PID=1021] arrived at node 2 | next node: 1
[PID=1022] arrived at node 3 | DESTINATION
[PID=1022] finished
[PID=1021] finished
```

### Relevant File
```txt
milestone5.c
```

### Compile
```bash
make milestone5
```

### Run
```bash
./sim graph.txt
```

---

## Milestone 6 — Node Synchronization with Semaphores

### Goal
Add synchronization so that no more than one traveler can be inside the same node at the same time.

### Features
- Each graph node is protected as a critical section.
- Only one traveler may occupy a node at a given time.
- Additional travelers wait outside the node.
- A traveler stays inside a node for one full second.
- Waiting travelers enter the node only after it becomes available.
- The GUI displays waiting travelers using a different visual state.
- Synchronization is implemented using POSIX semaphores.
- The solution prevents two travelers from being inside the same node simultaneously.
- Every waiting traveler eventually gets access, preventing starvation.

### Synchronization Mechanism
POSIX semaphores are used to protect each node.

Before a traveler enters a node, it must acquire the semaphore of that node. If the node is already occupied, the traveler waits outside. After the traveler finishes waiting inside the node, it releases the semaphore, allowing another waiting traveler to enter.

This implements mutual exclusion around node occupancy.

### Relevant File
```txt
milestone6.c
```

### Compile
```bash
make milestone6
```

### Run
```bash
./sim graph.txt
```

---

## Milestone 7 — Scheduling Algorithms

### Goal
Replace random node-entry order with a scheduling mechanism. When multiple travelers wait to enter the same node, the parent process chooses which traveler enters next according to the selected scheduling algorithm.

### Supported Scheduling Algorithms
This project supports two scheduling algorithms:

1. **FCFS — First Come, First Served**
   - Travelers enter the node according to arrival order.
   - The traveler that requested access first is selected first.

2. **SJF — Shortest Job First**
   - The scheduler gives preference to the traveler with the shorter required job/route criterion used in the implementation.
   - This can reduce waiting time for shorter tasks, but longer tasks may wait more.

### Features
- The scheduling algorithm is selected from the command line.
- The project supports running the same input file with different scheduling algorithms.
- The parent process manages waiting queues for nodes.
- The scheduler decides which waiting traveler may enter the node next.
- The GUI clearly displays which scheduling algorithm is currently active.
- The implementation demonstrates the difference between FCFS and SJF.

### Scheduler Comparison
- **FCFS** is fair and simple because travelers are handled according to arrival order.
- **SJF** may reduce the waiting time of shorter tasks, but can cause longer tasks to wait more if shorter tasks keep arriving.
- Running the same input file with both algorithms allows comparing how the selected policy affects waiting times and node-entry order.

### Relevant Files
```txt
milestone7/main_m7.c
milestone7/common.h
milestone7/graph.c
milestone7/graph.h
milestone7/dijkstra.c
milestone7/dijkstra.h
milestone7/ipc.c
milestone7/ipc.h
milestone7/scheduler.c
milestone7/scheduler.h
milestone7/gui.c
milestone7/gui.h
graph_m7.txt
```

### Compile
```bash
make milestone7
```

### Run with FCFS
```bash
./sim -schd fcfs graph_m7.txt
```

### Run with SJF
```bash
./sim -schd sjf graph_m7.txt
```

---

## Project Files

```txt
Makefile
README.md
graph.txt
graph_m7.txt
milestone1.c
milestone2.c
milestone3.c
milestone4.c
milestone5.c
milestone6.c
milestone7/
  common.h
  graph.c
  graph.h
  dijkstra.c
  dijkstra.h
  ipc.c
  ipc.h
  scheduler.c
  scheduler.h
  gui.c
  gui.h
  main_m7.c
```

---

## Input File Format

### Basic Format for Milestones 1–3
```txt
N M
src dst weight
...
source destination
```

### Example
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

### Extended Format for Multiple Travelers
Used in later milestones where the simulation supports several travelers.

```txt
N M
src dst weight
...
number_of_travelers
source1 destination1
source2 destination2
...
```

---

## Makefile Targets

```bash
make milestone1
make milestone2
make milestone3
make milestone4
make milestone5
make milestone6
make milestone7
make clean
```

---

## Cleaning Build Files

```bash
make clean
```

---

## Requirements

- Linux / Ubuntu environment
- GCC compiler
- raylib graphics library
- POSIX system calls
- Unix process support: `fork`, `wait`, `kill`, signals
- Unix IPC support: pipes
- POSIX semaphore support

---

## Technical Topics Covered

- Directed weighted graphs
- Dijkstra’s shortest path algorithm
- GUI programming with raylib
- Animation timing
- Multiple processes
- Parent-child process management
- Signals
- Inter-process communication using pipes
- Critical sections
- Mutual exclusion
- POSIX semaphores
- Waiting queues
- Scheduling algorithms
- FCFS scheduling
- SJF scheduling

---

## Team Members and Responsibilities

- Shahd Muhtaseb — Project setup and documentation
- Sara Zuheka — Dijkstra logic and GUI components
- Sarah Jweiles — Graph structures, validation, and visualization logic
- Hadeel Abbasi — Testing, Makefile updates, animation flow, and integration
practice push test
