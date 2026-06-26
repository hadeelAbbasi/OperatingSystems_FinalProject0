#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <raylib.h>
#include <time.h>

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
    pid_t pid;
    int travelerIndex;
    int currentNode;
    int nextNode;
    int finished;
    int noPath;
} Message;

typedef struct {
    int source;
    int dest;

    int currentNode;
    int nextNode;
    int finished;
    int noPath;

    Vector2 position;
    Color color;
    pid_t pid;
    int pipeFd[2];      // child -> parent
    int ackPipeFd[2];   // parent -> child confirmation
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

int dijkstraSilent(int N, int graph[MAX_NODES][MAX_NODES], int start, int end, int path[]) {
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
    DrawText("Graph GUI - Stage 5 IPC", 210, 20, 30, BLACK);

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

int allTravelersFinished(Traveler travelers[], int travelerCount) {
    for (int i = 0; i < travelerCount; i++) {
        if (!travelers[i].finished) {
            return 0;
        }
    }

    return 1;
}

void waitForParentConfirmation(int ackReadFd) {
    char confirmation;
    ssize_t bytesRead;

    do {
        bytesRead = read(ackReadFd, &confirmation, sizeof(confirmation));
    } while (bytesRead == -1 && errno == EINTR);
}

void sendMessage(int pipeWriteFd,
                 int ackReadFd,
                 int travelerIndex,
                 int currentNode,
                 int nextNode,
                 int finished,
                 int noPath) {
    Message msg;

    msg.pid = getpid();
    msg.travelerIndex = travelerIndex;
    msg.currentNode = currentNode;
    msg.nextNode = nextNode;
    msg.finished = finished;
    msg.noPath = noPath;

    write(pipeWriteFd, &msg, sizeof(Message));

    // The child waits for parent confirmation before continuing.
    waitForParentConfirmation(ackReadFd);
}
void sleepMilliseconds(int milliseconds) {
    struct timespec ts;

    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (long)(milliseconds % 1000) * 1000000L;

    nanosleep(&ts, NULL);
}
void childProcessWork(int travelerIndex,
                      int source,
                      int dest,
                      int N,
                      int graph[MAX_NODES][MAX_NODES],
                      Edge edges[],
                      int M,
                      int pipeWriteFd,
                      int ackReadFd) {
    int path[MAX_NODES];
    int pathLength = dijkstraSilent(N, graph, source, dest, path);

    if (pathLength <= 0) {
        sendMessage(pipeWriteFd, ackReadFd, travelerIndex, source, -1, 1, 1);
        close(pipeWriteFd);
        close(ackReadFd);
        exit(0);
    }

    for (int i = 0; i < pathLength; i++) {
        int currentNode = path[i];
        int nextNode = -1;
        int finished = 0;

        if (i == pathLength - 1) {
            finished = 1;
            nextNode = -1;
        } else {
            nextNode = path[i + 1];
        }

        sendMessage(pipeWriteFd, ackReadFd, travelerIndex, currentNode, nextNode, finished, 0);

        if (finished) {
            break;
        }

        if (i > 0) {
            sleep(1);
        }

        int weight = getEdgeWeight(edges, M, currentNode, nextNode);

        if (weight == INF || weight <= 0) {
            sendMessage(pipeWriteFd, ackReadFd, travelerIndex, currentNode, -1, 1, 1);
            close(pipeWriteFd);
            exit(0);
        }

sleepMilliseconds(weight * 300);
    }

    close(pipeWriteFd);
    close(ackReadFd);
    exit(0);
}

void handleIncomingMessages(Traveler travelers[],
                            int travelerCount,
                            Vector2 positions[]) {
    for (int i = 0; i < travelerCount; i++) {
        while (1) {
            Message msg;
            ssize_t bytesRead = read(travelers[i].pipeFd[0], &msg, sizeof(Message));

            if (bytesRead == sizeof(Message)) {
                int index = msg.travelerIndex;

                if (index < 0 || index >= travelerCount) {
                    continue;
                }

                // Parent sends confirmation back to the child after reading the message.
                char confirmation = 'A';
                write(travelers[index].ackPipeFd[1], &confirmation, sizeof(confirmation));

                travelers[index].currentNode = msg.currentNode;
                travelers[index].nextNode = msg.nextNode;
                travelers[index].position = positions[msg.currentNode];

                if (msg.noPath) {
                    printf("[PID=%d] No path found\n", msg.pid);
                    printf("[PID=%d] finished\n", msg.pid);
                    fflush(stdout);

                    travelers[index].noPath = 1;
                    travelers[index].finished = 1;
                    continue;
                }

                if (msg.finished) {
                    printf("[PID=%d] arrived at node %d | DESTINATION\n",
                           msg.pid,
                           msg.currentNode);
                    printf("[PID=%d] finished\n", msg.pid);
                    fflush(stdout);

                    travelers[index].finished = 1;
                } else {
                    printf("[PID=%d] arrived at node %d | next node: %d\n",
                           msg.pid,
                           msg.currentNode,
                           msg.nextNode);
                    fflush(stdout);
                }
            } else {
                if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    break;
                }

                if (bytesRead == 0) {
                    break;
                }

                break;
            }
        }
    }
}

void runSimulation(int N,
                   Edge edges[],
                   int M,
                   Traveler travelers[],
                   int travelerCount) {
    Vector2 positions[MAX_NODES];
    calculatePositions(N, positions);

    Color colors[MAX_TRAVELERS] = {
        ORANGE, GREEN, PURPLE, MAROON, DARKBLUE,
        GOLD, PINK, BROWN, RED, DARKGREEN
    };

    for (int i = 0; i < travelerCount; i++) {
        travelers[i].color = colors[i % MAX_TRAVELERS];
        travelers[i].currentNode = travelers[i].source;
        travelers[i].nextNode = -1;
        travelers[i].finished = 0;
        travelers[i].noPath = 0;
        travelers[i].position = positions[travelers[i].source];
    }

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Directed Weighted Graph - Stage 5 IPC");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        handleIncomingMessages(travelers, travelerCount, positions);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawStaticGraph(N, edges, M, positions);

        DrawText("Stage 5: child processes calculate paths + send updates using pipes",
                 110,
                 80,
                 20,
                 DARKGRAY);

        for (int i = 0; i < travelerCount; i++) {
            DrawCircleV(travelers[i].position, 14, travelers[i].color);
            DrawCircleLines((int)travelers[i].position.x,
                            (int)travelers[i].position.y,
                            14,
                            BLACK);

            DrawText(TextFormat("T%d", i + 1),
                     (int)travelers[i].position.x + 15,
                     (int)travelers[i].position.y - 10,
                     18,
                     travelers[i].color);

            if (travelers[i].noPath) {
                DrawText(TextFormat("T%d: %d -> %d  PID=%d  NO PATH",
                                    i + 1,
                                    travelers[i].source,
                                    travelers[i].dest,
                                    travelers[i].pid),
                         20,
                         590 + i * 25,
                         16,
                         RED);
            } else if (travelers[i].finished) {
                DrawText(TextFormat("T%d: %d -> %d  PID=%d  DONE",
                                    i + 1,
                                    travelers[i].source,
                                    travelers[i].dest,
                                    travelers[i].pid),
                         20,
                         590 + i * 25,
                         16,
                         DARKGREEN);
            } else {
                DrawText(TextFormat("T%d: %d -> %d  PID=%d  current=%d next=%d",
                                    i + 1,
                                    travelers[i].source,
                                    travelers[i].dest,
                                    travelers[i].pid,
                                    travelers[i].currentNode,
                                    travelers[i].nextNode),
                         20,
                         590 + i * 25,
                         16,
                         travelers[i].color);
            }
        }

        if (allTravelersFinished(travelers, travelerCount)) {
            DrawText("All travelers finished!", 300, 115, 28, DARKGREEN);
        }

        EndDrawing();

        if (allTravelersFinished(travelers, travelerCount)) {
            WaitTime(2.0);
            break;
        }
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

        travelers[i].pid = -1;
        travelers[i].finished = 0;
        travelers[i].noPath = 0;
        travelers[i].currentNode = travelers[i].source;
        travelers[i].nextNode = -1;
        travelers[i].pipeFd[0] = -1;
        travelers[i].pipeFd[1] = -1;
        travelers[i].ackPipeFd[0] = -1;
        travelers[i].ackPipeFd[1] = -1;
    }

    fclose(file);

    for (int i = 0; i < travelerCount; i++) {
        if (pipe(travelers[i].pipeFd) == -1) {
            printf("pipe failed\n");
            return 1;
        }

        if (pipe(travelers[i].ackPipeFd) == -1) {
            printf("ack pipe failed\n");
            return 1;
        }

        pid_t pid = fork();

        if (pid < 0) {
            printf("fork failed\n");
            return 1;
        }

        if (pid == 0) {
            close(travelers[i].pipeFd[0]);
            close(travelers[i].ackPipeFd[1]);

            childProcessWork(i,
                             travelers[i].source,
                             travelers[i].dest,
                             N,
                             graph,
                             edges,
                             M,
                             travelers[i].pipeFd[1],
                             travelers[i].ackPipeFd[0]);
        }

        close(travelers[i].pipeFd[1]);
        close(travelers[i].ackPipeFd[0]);

        int flags = fcntl(travelers[i].pipeFd[0], F_GETFL, 0);

        if (flags == -1) {
            printf("fcntl failed\n");
            return 1;
        }

        if (fcntl(travelers[i].pipeFd[0], F_SETFL, flags | O_NONBLOCK) == -1) {
            printf("fcntl failed\n");
            return 1;
        }

        travelers[i].pid = pid;
    }

    runSimulation(N, edges, M, travelers, travelerCount);

    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].pipeFd[0] != -1) {
            close(travelers[i].pipeFd[0]);
        }

        if (travelers[i].ackPipeFd[1] != -1) {
            close(travelers[i].ackPipeFd[1]);
        }
    }

    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].pid > 0) {
            int status;
            pid_t result = waitpid(travelers[i].pid, &status, WNOHANG);

            if (result == 0) {
                kill(travelers[i].pid, SIGTERM);
                waitpid(travelers[i].pid, NULL, 0);
            }
        }
    }

    return 0;
}