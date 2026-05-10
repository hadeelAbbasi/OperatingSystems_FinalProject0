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

## Project Files
```txt
milestone1.c
milestone2.c
milestone3.c
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
