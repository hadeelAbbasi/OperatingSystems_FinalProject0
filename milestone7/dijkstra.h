#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "common.h"

int dijkstra(int N, int graph[MAX_NODES][MAX_NODES],
             int start, int end, int path[]);

int pathRemainingCost(int path[], int pathLength,
                      int fromIndex, Edge edges[], int M);

#endif