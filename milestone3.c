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

int dijkstra(int N, int graph[MAX_NODES][MAX_NODES], int start, int end, int path[]) {
    if (start == end) {
        path[0] = start;
        printf("%d\n0\n", start);
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
        printf("No path found\n");
        return 0;
    }

    int reversed[MAX_NODES];
    int length = 0;
    int current = end;

    while (current != -1) {
        reversed[length++] = current;
        current = parent[current];
    }

    for (int i = 0; i < length; i++) {
        path[i] = reversed[length - 1 - i];
    }

    for (int i = 0; i < length; i++) {
        printf("%d", path[i]);
        if (i < length - 1) {
            printf(" -> ");
        }
    }

    printf("\n%d\n", dist[end]);

    return length;
}

int getEdgeWeight(Edge edges[], int M, int src, int dst) {
    for (int i = 0; i < M; i++) {
        if (edges[i].src == src && edges[i].dst == dst) {
            return edges[i].weight;
        }
    }

    return INF;
}

void calculatePositions(int N, Vector2 positions[]) {
    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = 360.0f;
    float radiusX = 240.0f;
    float radiusY = 125.0f;

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

void drawStaticGraph(int N, Edge edges[], int M, Vector2 positions[]) {
    DrawText("Graph GUI - Stage 3 Animation", 200, 20, 30, BLACK);

    for (int i = 0; i < M; i++) {
        int src = edges[i].src;
        int dst = edges[i].dst;
        int weight = edges[i].weight;

        DrawArrow(positions[src], positions[dst], DARKGRAY);

        Vector2 mid = {
            (positions[src].x + positions[dst].x) / 2.0f,
            (positions[src].y + positions[dst].y) / 2.0f
        };

        DrawCircleV(mid, 13, RAYWHITE);
        DrawText(TextFormat("%d", weight),
                 (int)mid.x - 6,
                 (int)mid.y - 10,
                 20,
                 RED);
    }

    for (int i = 0; i < N; i++) {
        DrawCircleV(positions[i], 25, SKYBLUE);
        DrawCircleLines((int)positions[i].x, (int)positions[i].y, 25, DARKBLUE);

        DrawText(TextFormat("%d", i),
                 (int)positions[i].x - 6,
                 (int)positions[i].y - 10,
                 20,
                 BLACK);
    }
}

void runSimulation(int N, Edge edges[], int M, int path[], int pathLength) {
    Vector2 positions[MAX_NODES];
    calculatePositions(N, positions);

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Directed Weighted Graph - Stage 3");
    SetTargetFPS(60);

    Rectangle button = { 20, 20, 120, 45 };

    int playing = 0;
    int finished = 0;
    int waiting = 0;

    int segmentIndex = 0;
    int jumpIndex = 0;
    float timer = 0.0f;

    Vector2 entityPosition = positions[path[0]];

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();

            if (CheckCollisionPointRec(mouse, button) && !finished) {
                playing = !playing;
            }
        }

        if (playing && !finished && pathLength > 1) {
            timer += dt;

            if (waiting) {
                if (timer >= 1.0f) {
                    waiting = 0;
                    timer = 0.0f;
                }
            } else {
                int from = path[segmentIndex];
                int to = path[segmentIndex + 1];
                int weight = getEdgeWeight(edges, M, from, to);

                if (weight <= 0 || weight == INF) {
                    finished = 1;
                    playing = 0;
                } else if (timer >= 0.3f) {
                    timer = 0.0f;
                    jumpIndex++;

                    float t = (float)jumpIndex / weight;

                    if (t > 1.0f) {
                        t = 1.0f;
                    }

                    entityPosition.x = positions[from].x + (positions[to].x - positions[from].x) * t;
                    entityPosition.y = positions[from].y + (positions[to].y - positions[from].y) * t;

                    if (jumpIndex >= weight) {
                        segmentIndex++;
                        jumpIndex = 0;

                        if (segmentIndex >= pathLength - 1) {
                            entityPosition = positions[path[pathLength - 1]];
                            finished = 1;
                            playing = 0;
                        } else {
                            waiting = 1;
                            timer = 0.0f;
                        }
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawStaticGraph(N, edges, M, positions);

        DrawRectangleRec(button, playing ? RED : GREEN);
        DrawText(playing ? "STOP" : "PLAY", 48, 32, 22, WHITE);

        DrawText("Click PLAY/STOP to control animation", 230, 80, 20, DARKGRAY);

        if (finished) {
            DrawText("Arrived at destination!", 250, 115, 28, DARKGREEN);
        }

        DrawCircleV(entityPosition, 14, ORANGE);
        DrawCircleLines((int)entityPosition.x, (int)entityPosition.y, 14, MAROON);

        EndDrawing();
    }

    CloseWindow();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./sim <file_name>\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    int N, M;

    if (fscanf(file, "%d %d", &N, &M) != 2) {
        printf("Invalid input\n");
        fclose(file);
        return 1;
    }

    if (N <= 0 || N > MAX_NODES || M < 0 || M > MAX_EDGES) {
        printf("Invalid input\n");
        fclose(file);
        return 1;
    }

    int graph[MAX_NODES][MAX_NODES];
    Edge edges[MAX_EDGES];

    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            graph[i][j] = INF;
        }
    }

    for (int i = 0; i < N; i++) {
        graph[i][i] = 0;
    }

    for (int i = 0; i < M; i++) {
        int src, dst, weight;

        if (fscanf(file, "%d %d %d", &src, &dst, &weight) != 3) {
            printf("Invalid input\n");
            fclose(file);
            return 1;
        }

        if (src < 0 || dst < 0 || src >= N || dst >= N || weight <= 0) {
            printf("Invalid input\n");
            fclose(file);
            return 1;
        }

        graph[src][dst] = weight;

        edges[i].src = src;
        edges[i].dst = dst;
        edges[i].weight = weight;
    }

    int start, end;

    if (fscanf(file, "%d %d", &start, &end) != 2) {
        printf("Invalid input\n");
        fclose(file);
        return 1;
    }

    if (start < 0 || end < 0 || start >= N || end >= N) {
        printf("Invalid input\n");
        fclose(file);
        return 1;
    }

    fclose(file);

    int path[MAX_NODES];
    int pathLength = dijkstra(N, graph, start, end, path);

    if (pathLength > 0) {
        runSimulation(N, edges, M, path, pathLength);
    }

    return 0;
}