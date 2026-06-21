#include <stdio.h>
#include "graph.h"

void initGraphMatrix(int graph[MAX_NODES][MAX_NODES]) {
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            graph[i][j] = INF;
        }
    }

    for (int i = 0; i < MAX_NODES; i++) {
        graph[i][i] = 0;
    }
}

int getEdgeWeight(Edge edges[], int M, int src, int dst) {
    for (int i = 0; i < M; i++) {
        if (edges[i].src == src && edges[i].dst == dst) {
            return edges[i].weight;
        }
    }

    return INF;
}

int readInputFile(const char *fileName, GraphData *data) {
    FILE *file = fopen(fileName, "r");

    if (!file) {
        printf("Error opening file\n");
        return 0;
    }

    if (fscanf(file, "%d %d", &data->N, &data->M) != 2) {
        printf("Invalid input\n");
        fclose(file);
        return 0;
    }

    if (data->N <= 0 || data->N > MAX_NODES ||
        data->M < 0 || data->M > MAX_EDGES) {
        printf("Invalid input\n");
        fclose(file);
        return 0;
    }

    initGraphMatrix(data->matrix);

    for (int i = 0; i < data->M; i++) {
        int src, dst, weight;

        if (fscanf(file, "%d %d %d", &src, &dst, &weight) != 3) {
            printf("Invalid input\n");
            fclose(file);
            return 0;
        }

        if (src < 0 || dst < 0 ||
            src >= data->N || dst >= data->N ||
            weight <= 0) {
            printf("Invalid input\n");
            fclose(file);
            return 0;
        }

        data->matrix[src][dst] = weight;

        data->edges[i].src = src;
        data->edges[i].dst = dst;
        data->edges[i].weight = weight;
    }

    if (fscanf(file, "%d", &data->travelerCount) != 1) {
        printf("Invalid travelers input\n");
        fclose(file);
        return 0;
    }

    if (data->travelerCount <= 0 || data->travelerCount > MAX_TRAVELERS) {
        printf("Invalid travelers count\n");
        fclose(file);
        return 0;
    }

    for (int i = 0; i < data->travelerCount; i++) {
        Traveler *t = &data->travelers[i];

        if (fscanf(file, "%d %d", &t->source, &t->dest) != 2) {
            printf("Invalid traveler input\n");
            fclose(file);
            return 0;
        }

        if (t->source < 0 || t->dest < 0 ||
            t->source >= data->N || t->dest >= data->N) {
            printf("Invalid traveler input\n");
            fclose(file);
            return 0;
        }

        t->pid = -1;
        t->finished = 0;
        t->noPath = 0;
        t->waiting = 0;
        t->waitingNode = -1;
        t->moving = 0;
        t->entering = 0;
        t->currentNode = t->source;
        t->nextNode = -1;

        t->childToParent[0] = -1;
        t->childToParent[1] = -1;
        t->parentToChild[0] = -1;
        t->parentToChild[1] = -1;
    }

    fclose(file);
    return 1;
}