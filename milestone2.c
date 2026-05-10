#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <raylib.h>

#define INF 1000000000
#define MAX_NODES 15
#define MAX_EDGES 200
#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 700

typedef struct {
    int src;
    int dst;
    int weight;
} Edge;

int minDistance(int dist[], int visited[], int N) {
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

void dijkstra(int N, int graph[MAX_NODES][MAX_NODES], int start, int end) {
    if (start == end) {
        printf("%d\n0\n", start);
        return;
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
        printf("No path found\n");
        return;
    }

    int path[MAX_NODES];
    int pathLength = 0;
    int current = end;

    while (current != -1) {
        path[pathLength++] = current;
        current = parent[current];
    }

    for (int i = pathLength - 1; i >= 0; i--) {
        printf("%d", path[i]);

        if (i > 0) {
            printf(" -> ");
        }
    }

    printf("\n%d\n", dist[end]);
}
void calculatePositions(int N, Vector2 positions[]) {
    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = 360.0f;
    float radiusX = 260.0f;
    float radiusY = 160.0f;

    for (int i = 0; i < N; i++) {
        float angle = 2.0f * PI * i / N - PI / 2.0f;

        positions[i].x = centerX + radiusX * cosf(angle);
        positions[i].y = centerY + radiusY * sinf(angle);
    }
}


void DrawArrow(Vector2 start, Vector2 end, Color color) {
    float nodeRadius = 25.0f;

    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float length = sqrtf(dx * dx + dy * dy);

    if (length == 0) {
        return;
    }

    float ux = dx / length;
    float uy = dy / length;

    Vector2 lineStart = {
        start.x + ux * nodeRadius,
        start.y + uy * nodeRadius
    };

    Vector2 lineEnd = {
        end.x - ux * (nodeRadius + 5),
        end.y - uy * (nodeRadius + 5)
    };

    DrawLineEx(lineStart, lineEnd, 4, color);

    float arrowSize = 28.0f;
    float angle = atan2f(dy, dx);

    Vector2 left = {
        lineEnd.x - arrowSize * cosf(angle - PI / 5),
        lineEnd.y - arrowSize * sinf(angle - PI / 5)
    };

    Vector2 right = {
        lineEnd.x - arrowSize * cosf(angle + PI / 5),
        lineEnd.y - arrowSize * sinf(angle + PI / 5)
    };

    DrawLineEx(lineEnd, left, 5, RED);
    DrawLineEx(lineEnd, right, 5, RED);
}