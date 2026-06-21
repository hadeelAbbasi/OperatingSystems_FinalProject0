#include "dijkstra.h"
#include "graph.h"

static int minDistance(int dist[], int visited[], int N) {
    int min = INF;
    int minIndex = -1;

    for (int i = 0; i < N; i++) {
        if (!visited[i] && dist[i] < min) {
            min = dist[i];
            minIndex = i;
        }
    }

    return minIndex;
}

int dijkstra(int N, int graph[MAX_NODES][MAX_NODES],
             int start, int end, int path[]) {
    if (start == end) {
        path[0] = start;
        return 1;
    }

    int dist[MAX_NODES];
    int visited[MAX_NODES];
    int parent[MAX_NODES];

    for (int i = 0; i < N; i++) {
        dist[i] = INF;
        visited[i] = 0;
        parent[i] = -1;
    }

    dist[start] = 0;

    for (int count = 0; count < N - 1; count++) {
        int u = minDistance(dist, visited, N);

        if (u == -1) {
            break;
        }

        visited[u] = 1;

        for (int v = 0; v < N; v++) {
            if (!visited[v] &&
                graph[u][v] != INF &&
                dist[u] != INF &&
                dist[u] + graph[u][v] < dist[v]) {

                dist[v] = dist[u] + graph[u][v];
                parent[v] = u;
                }
        }
    }

    if (dist[end] == INF) {
        return 0;
    }

    int reversed[MAX_PATH];
    int length = 0;
    int current = end;

    while (current != -1 && length < MAX_PATH) {
        reversed[length++] = current;
        current = parent[current];
    }

    for (int i = 0; i < length; i++) {
        path[i] = reversed[length - 1 - i];
    }

    return length;
}

int pathRemainingCost(int path[], int pathLength,
                      int fromIndex, Edge edges[], int M) {
    int sum = 0;

    for (int i = fromIndex; i < pathLength - 1; i++) {
        int w = getEdgeWeight(edges, M, path[i], path[i + 1]);

        if (w == INF) {
            return INF;
        }

        sum += w;
    }

    return sum;
}