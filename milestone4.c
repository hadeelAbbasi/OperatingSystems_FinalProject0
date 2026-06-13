#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <raylib.h>

#define INF 1000000000
#define MAX_NODES 15
#define MAX_EDGES 200
#define MAX_TRAVELERS 10
#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 700

typedef struct {
    int src;
    int dst;
    int weight;
} Edge;

typedef struct {
    int source;
    int dest;
    int path[MAX_NODES];
    int pathLength;

    int segmentIndex;
    int jumpIndex;
    float timer;
    int waiting;
    int finished;

    Vector2 position;
    Color color;
    pid_t pid;
} Traveler;

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
    DrawText("Graph GUI - Stage 4 Multiple Travelers", 150, 20, 30, BLACK);

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

void updateTraveler(Traveler *traveler, Edge edges[], int M, Vector2 positions[], float dt) {
    if (traveler->finished || traveler->pathLength <= 0) {
        return;
    }

    if (traveler->pathLength == 1) {
        traveler->finished = 1;
        kill(traveler->pid, SIGTERM);
        return;
    }

    traveler->timer += dt;

    if (traveler->waiting) {
        if (traveler->timer >= 1.0f) {
            traveler->waiting = 0;
            traveler->timer = 0.0f;
        }
        return;
    }

    int from = traveler->path[traveler->segmentIndex];
    int to = traveler->path[traveler->segmentIndex + 1];
    int weight = getEdgeWeight(edges, M, from, to);

    if (weight <= 0 || weight == INF) {
        traveler->finished = 1;
        kill(traveler->pid, SIGTERM);
        return;
    }

    if (traveler->timer >= 0.3f) {
        traveler->timer = 0.0f;
        traveler->jumpIndex++;

        float t = (float)traveler->jumpIndex / weight;

        if (t > 1.0f) {
            t = 1.0f;
        }

        traveler->position.x = positions[from].x + (positions[to].x - positions[from].x) * t;
        traveler->position.y = positions[from].y + (positions[to].y - positions[from].y) * t;

        if (traveler->jumpIndex >= weight) {
            traveler->segmentIndex++;
            traveler->jumpIndex = 0;

            if (traveler->segmentIndex >= traveler->pathLength - 1) {
                traveler->position = positions[traveler->path[traveler->pathLength - 1]];
                traveler->finished = 1;
                kill(traveler->pid, SIGTERM);
            } else {
                traveler->waiting = 1;
                traveler->timer = 0.0f;
            }
        }
    }
}

int allTravelersFinished(Traveler travelers[], int travelerCount) {
    for (int i = 0; i < travelerCount; i++) {
        if (!travelers[i].finished && travelers[i].pathLength > 0) {
            return 0;
        }
    }

    return 1;
}

void runSimulation(int N, Edge edges[], int M, Traveler travelers[], int travelerCount) {
    Vector2 positions[MAX_NODES];
    calculatePositions(N, positions);

    Color colors[MAX_TRAVELERS] = {
        ORANGE, GREEN, PURPLE, MAROON, DARKBLUE,
        GOLD, PINK, BROWN, RED, DARKGREEN
    };

    for (int i = 0; i < travelerCount; i++) {
        travelers[i].segmentIndex = 0;
        travelers[i].jumpIndex = 0;
        travelers[i].timer = 0.0f;
        travelers[i].waiting = 0;
        travelers[i].finished = 0;
        travelers[i].color = colors[i % MAX_TRAVELERS];

        if (travelers[i].pathLength > 0) {
            travelers[i].position = positions[travelers[i].path[0]];
        }
    }

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Directed Weighted Graph - Stage 4");
    SetTargetFPS(60);

    Rectangle button = { 20, 20, 120, 45 };
    int playing = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();

            if (CheckCollisionPointRec(mouse, button) && !allTravelersFinished(travelers, travelerCount)) {
                playing = !playing;
            }
        }

        if (playing) {
            for (int i = 0; i < travelerCount; i++) {
                updateTraveler(&travelers[i], edges, M, positions, dt);
            }

            if (allTravelersFinished(travelers, travelerCount)) {
                playing = 0;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawStaticGraph(N, edges, M, positions);

        DrawRectangleRec(button, playing ? RED : GREEN);
        DrawText(playing ? "STOP" : "PLAY", 48, 32, 22, WHITE);

        DrawText("Stage 4: parent GUI + forked child processes", 210, 80, 20, DARKGRAY);

        for (int i = 0; i < travelerCount; i++) {
            if (travelers[i].pathLength <= 0) {
                continue;
            }

            DrawCircleV(travelers[i].position, 14, travelers[i].color);
            DrawCircleLines((int)travelers[i].position.x, (int)travelers[i].position.y, 14, BLACK);

            DrawText(TextFormat("T%d", i + 1),
                     (int)travelers[i].position.x + 15,
                     (int)travelers[i].position.y - 10,
                     18,
                     travelers[i].color);

            DrawText(TextFormat("T%d: %d -> %d  PID=%d",
                                i + 1,
                                travelers[i].source,
                                travelers[i].dest,
                                travelers[i].pid),
                     20,
                     610 + i * 18,
                     16,
                     travelers[i].color);
        }

        if (allTravelersFinished(travelers, travelerCount)) {
            DrawText("All travelers arrived!", 300, 115, 28, DARKGREEN);
        }

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

    int travelerCount;

    if (fscanf(file, "%d", &travelerCount) != 1) {
        printf("Invalid travelers input\n");
        fclose(file);
        return 1;
    }

    if (travelerCount <= 0 || travelerCount > MAX_TRAVELERS) {
        printf("Invalid travelers count\n");
        fclose(file);
        return 1;
    }

    Traveler travelers[MAX_TRAVELERS];

    for (int i = 0; i < travelerCount; i++) {
        if (fscanf(file, "%d %d", &travelers[i].source, &travelers[i].dest) != 2) {
            printf("Invalid traveler input\n");
            fclose(file);
            return 1;
        }

        if (travelers[i].source < 0 || travelers[i].dest < 0 ||
            travelers[i].source >= N || travelers[i].dest >= N) {
            printf("Invalid traveler input\n");
            fclose(file);
            return 1;
        }
    }

    fclose(file);

    for (int i = 0; i < travelerCount; i++) {
        travelers[i].pathLength = dijkstra(N,
                                           graph,
                                           travelers[i].source,
                                           travelers[i].dest,
                                           travelers[i].path);

        if (travelers[i].pathLength <= 0) {
            travelers[i].finished = 1;
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            printf("fork failed\n");
            return 1;
        }

        if (pid == 0) {
            printf("[%d] started\n", getpid());
            fflush(stdout);

            while (1) {
                pause();
            }

            return 0;
        }

        travelers[i].pid = pid;
    }

    runSimulation(N, edges, M, travelers, travelerCount);

    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].pathLength > 0) {
            kill(travelers[i].pid, SIGTERM);
            waitpid(travelers[i].pid, NULL, 0);
        }
    }

    return 0;
}