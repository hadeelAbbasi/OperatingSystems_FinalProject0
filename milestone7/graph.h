#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"

typedef struct {
    int N;
    int M;
    int matrix[MAX_NODES][MAX_NODES];
    Edge edges[MAX_EDGES];

    int travelerCount;
    Traveler travelers[MAX_TRAVELERS];
} GraphData;

void initGraphMatrix(int graph[MAX_NODES][MAX_NODES]);
int readInputFile(const char *fileName, GraphData *data);
int getEdgeWeight(Edge edges[], int M, int src, int dst);

#endif