#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <raylib.h>
#include <time.h>

#define INF 1000000000
#define MAX_NODES 15
#define MAX_EDGES 200
#define MAX_TRAVELERS 10
#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 700

#define STATUS_ARRIVED 1
#define STATUS_WAITING 2
#define STATUS_FINISHED 3
#define STATUS_NO_PATH 4
#define STATUS_MOVING 5
#define EDGE_TIME_SCALE 1500
#define ENTER_ANIMATION_MS 120

typedef struct {
    int src;
    int dst;
    int weight;
} Edge;

typedef struct {
    pid_t pid;
    int travelerIndex;
    int previousNode;
    int currentNode;
    int nextNode;
    int status;
} Message;

typedef struct {
    int source;
    int dest;
    int currentNode;
    int nextNode;
    int finished;
    int noPath;
    int waiting;
    int waitingNode;
    int moving;
    int entering;
    int stopBeforeNextNode;

    Vector2 position;

    Vector2 moveStartPosition;
    Vector2 moveTargetPosition;
    double moveStartTime;
    double moveDuration;

    Vector2 enterStartPosition;
    Vector2 enterTargetPosition;
    double enterStartTime;
    double enterDuration;

    Color color;

    pid_t pid;
    int pipeFd[2];
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
    Vector2 custom[MAX_NODES] = {
        {450, 170},
        {660, 260},
        {610, 440},
        {380, 520},
        {180, 420},
        {180, 250}
    };

    if (N == 6) {
        for (int i = 0; i < N; i++) {
            positions[i] = custom[i];
        }
        return;
    }

    float centerX = SCREEN_WIDTH / 2.0f;
    float centerY = 360.0f;
    float radius = 230.0f;

    for (int i = 0; i < N; i++) {
        float angle = 2.0f * PI * i / N - PI / 2.0f;
        positions[i].x = centerX + radius * cosf(angle);
        positions[i].y = centerY + radius * sinf(angle);
    }
}

void DrawArrow(Vector2 start, Vector2 end, Color color) {
    float nodeRadius = 32.0f;

    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float length = sqrtf(dx * dx + dy * dy);

    if (length == 0) return;

    float ux = dx / length;
    float uy = dy / length;

    Vector2 lineStart = {
        start.x + ux * nodeRadius,
        start.y + uy * nodeRadius
    };

    Vector2 lineEnd = {
        end.x - ux * (nodeRadius + 10),
        end.y - uy * (nodeRadius + 10)
    };

    DrawLineEx(lineStart, lineEnd, 3, color);

    float arrowSize = 18.0f;
    float angle = atan2f(dy, dx);

    Vector2 left = {
        lineEnd.x - arrowSize * cosf(angle - PI / 5),
        lineEnd.y - arrowSize * sinf(angle - PI / 5)
    };

    Vector2 right = {
        lineEnd.x - arrowSize * cosf(angle + PI / 5),
        lineEnd.y - arrowSize * sinf(angle + PI / 5)
    };

    DrawLineEx(lineEnd, left, 4, color);
    DrawLineEx(lineEnd, right, 4, color);
}

void drawStaticGraph(int N, Edge edges[], int M, Vector2 positions[]) {
    Color bg = (Color){250, 252, 255, 255};
    Color text = (Color){30, 38, 55, 255};
    Color edgeColor = (Color){85, 95, 110, 255};
    Color nodeColor = (Color){65, 145, 220, 255};
    Color nodeBorder = (Color){25, 80, 140, 255};

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bg);

    DrawText("Directed Graph Traffic Simulation", 250, 25, 24, (Color){35, 45, 65, 255});
    DrawText("Milestone 6 - Node Synchronization", 325, 58, 14, DARKGRAY);

    for (int i = 0; i < M; i++) {
        int src = edges[i].src;
        int dst = edges[i].dst;
        int weight = edges[i].weight;

        DrawArrow(positions[src], positions[dst], edgeColor);

        Vector2 mid = {
            (positions[src].x + positions[dst].x) / 2.0f,
            (positions[src].y + positions[dst].y) / 2.0f
        };

        DrawRectangleRounded((Rectangle){mid.x - 16, mid.y - 13, 32, 26},
                             0.4f, 8, (Color){255, 255, 255, 245});

        DrawText(TextFormat("%d", weight), (int)mid.x - 6, (int)mid.y - 9, 18, text);
    }

    for (int i = 0; i < N; i++) {
        DrawCircleV((Vector2){positions[i].x + 4, positions[i].y + 5},
                    31, (Color){0, 0, 0, 35});

        DrawCircleV(positions[i], 30, nodeColor);
        DrawCircleLines((int)positions[i].x, (int)positions[i].y, 30, nodeBorder);

        DrawText(TextFormat("%d", i),
                 (int)positions[i].x - 6,
                 (int)positions[i].y - 11,
                 22,
                 WHITE);
    }
}

Vector2 waitingPositionNearNode(Vector2 nodePosition, int travelerIndex) {
    int slot = travelerIndex % 4;
    Vector2 offsets[4] = {
        {-45, -45},
        {45, -45},
        {-45, 45},
        {45, 45}
    };

    return (Vector2){
        nodePosition.x + offsets[slot].x,
        nodePosition.y + offsets[slot].y
    };
}

Vector2 waitingPositionOnIncomingEdge(Vector2 previousNodePosition,
                                      Vector2 targetNodePosition,
                                      int travelerIndex) {
    float dx = previousNodePosition.x - targetNodePosition.x;
    float dy = previousNodePosition.y - targetNodePosition.y;
    float length = sqrtf(dx * dx + dy * dy);

    if (length == 0) {
        return waitingPositionNearNode(targetNodePosition, travelerIndex);
    }

    dx /= length;
    dy /= length;

    float distanceFromNode = 58.0f + (travelerIndex % 3) * 18.0f;

    return (Vector2){
        targetNodePosition.x + dx * distanceFromNode,
        targetNodePosition.y + dy * distanceFromNode
    };
}

int allTravelersFinished(Traveler travelers[], int travelerCount) {
    for (int i = 0; i < travelerCount; i++) {
        if (!travelers[i].finished) return 0;
    }
    return 1;
}

void sendMessage(int pipeWriteFd,
                 int travelerIndex,
                 int previousNode,
                 int currentNode,
                 int nextNode,
                 int status) {
    Message msg;

    msg.pid = getpid();
    msg.travelerIndex = travelerIndex;
    msg.previousNode = previousNode;
    msg.currentNode = currentNode;
    msg.nextNode = nextNode;
    msg.status = status;

    write(pipeWriteFd, &msg, sizeof(Message));
}

void sleepMilliseconds(int milliseconds) {
    struct timespec ts;

    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (long)(milliseconds % 1000) * 1000000L;

    nanosleep(&ts, NULL);
}

void enterNodeWithSemaphore(sem_t *nodeSemaphores[],
                            int pipeWriteFd,
                            int travelerIndex,
                            int previousNode,
                            int currentNode,
                            int nextNode,
                            int isDestination) {
    if (sem_trywait(nodeSemaphores[currentNode]) == -1) {
        if (errno == EAGAIN) {
            sendMessage(pipeWriteFd,
                        travelerIndex,
                        previousNode,
                        currentNode,
                        nextNode,
                        STATUS_WAITING);

            sem_wait(nodeSemaphores[currentNode]);
        } else {
            sendMessage(pipeWriteFd,
                        travelerIndex,
                        previousNode,
                        currentNode,
                        nextNode,
                        STATUS_NO_PATH);
            return;
        }
    }

    sendMessage(pipeWriteFd,
                travelerIndex,
                previousNode,
                currentNode,
                nextNode,
                STATUS_ARRIVED);

    /*
     * The GUI uses a very short ENTERING animation from the edge entrance
     * into the node center. Keep the semaphore locked during this animation,
     * then keep the traveler one full second inside the node.
     */
    sleepMilliseconds(ENTER_ANIMATION_MS);
    sleep(1);

    if (isDestination) {
        sendMessage(pipeWriteFd,
                    travelerIndex,
                    previousNode,
                    currentNode,
                    nextNode,
                    STATUS_FINISHED);
    } else {
        sendMessage(pipeWriteFd,
                    travelerIndex,
                    previousNode,
                    currentNode,
                    nextNode,
                    STATUS_MOVING);

        /*
         * Keep the semaphore locked for a short exit animation.
         * This prevents the next waiting traveler from being shown inside
         * the same node while the current traveler is still visually touching
         * the node center.
         */
        sleepMilliseconds(120);
    }

    sem_post(nodeSemaphores[currentNode]);
}

void childProcessWork(int travelerIndex,
                      int source,
                      int dest,
                      int N,
                      int graph[MAX_NODES][MAX_NODES],
                      Edge edges[],
                      int M,
                      int pipeWriteFd,
                      sem_t *nodeSemaphores[]) {
    int path[MAX_NODES];
    int pathLength = dijkstraSilent(N, graph, source, dest, path);

    if (pathLength <= 0) {
        sendMessage(pipeWriteFd, travelerIndex, -1, source, -1, STATUS_NO_PATH);
        close(pipeWriteFd);
        exit(0);
    }

    for (int i = 0; i < pathLength; i++) {
        int previousNode = -1;
        int currentNode = path[i];
        int nextNode = -1;
        int isDestination = (i == pathLength - 1);

        if (i > 0) previousNode = path[i - 1];
        if (!isDestination) nextNode = path[i + 1];

        enterNodeWithSemaphore(nodeSemaphores,
                               pipeWriteFd,
                               travelerIndex,
                               previousNode,
                               currentNode,
                               nextNode,
                               isDestination);

        if (isDestination) break;

        int weight = getEdgeWeight(edges, M, currentNode, nextNode);

        if (weight == INF || weight <= 0) {
            sendMessage(pipeWriteFd,
                        travelerIndex,
                        previousNode,
                        currentNode,
                        -1,
                        STATUS_NO_PATH);
            close(pipeWriteFd);
            exit(0);
        }

        int travelTime = weight * EDGE_TIME_SCALE - ENTER_ANIMATION_MS;
        if (travelTime < 1) travelTime = 1;

        sleepMilliseconds(travelTime);
    }

    close(pipeWriteFd);
    exit(0);
}

int shouldMoveOnlyToEntrance(Traveler travelers[],
                              int travelerCount,
                              int travelerIndex,
                              int targetNode);

void handleIncomingMessages(Traveler travelers[],
                            int travelerCount,
                            Vector2 positions[],
                            Edge edges[],
                            int M) {
    Message pending[256];
    int pendingCount = 0;
    Message movingMessages[256];
    int movingCount = 0;

    for (int i = 0; i < travelerCount; i++) {
        while (1) {
            Message msg;
            ssize_t bytesRead = read(travelers[i].pipeFd[0], &msg, sizeof(Message));

            if (bytesRead == sizeof(Message)) {
                if (pendingCount < 256) {
                    pending[pendingCount++] = msg;
                }
            } else {
                if (bytesRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
                if (bytesRead == 0) break;
                break;
            }
        }
    }

    for (int p = 0; p < pendingCount; p++) {
        Message msg = pending[p];
        int index = msg.travelerIndex;

        if (index < 0 || index >= travelerCount) continue;

        travelers[index].pid = msg.pid;

        if (msg.status == STATUS_MOVING) {
            if (movingCount < 256) {
                movingMessages[movingCount++] = msg;
            }
            continue;
        }

        if (msg.status == STATUS_WAITING) {
            travelers[index].waiting = 1;
            travelers[index].moving = 0;
            travelers[index].entering = 0;
            travelers[index].stopBeforeNextNode = 0;
            travelers[index].waitingNode = msg.currentNode;

            if (msg.previousNode >= 0) {
                travelers[index].currentNode = msg.previousNode;
            }

            travelers[index].nextNode = msg.currentNode;

            if (msg.previousNode >= 0 &&
                msg.previousNode < MAX_NODES &&
                msg.currentNode >= 0 &&
                msg.currentNode < MAX_NODES) {

                travelers[index].position =
                    waitingPositionOnIncomingEdge(positions[msg.previousNode],
                                                  positions[msg.currentNode],
                                                  index);
            } else {
                travelers[index].position =
                    waitingPositionNearNode(positions[msg.currentNode], index);
            }

            printf("[PID=%d] waiting outside node %d\n", msg.pid, msg.currentNode);
            fflush(stdout);

        } else if (msg.status == STATUS_ARRIVED) {
            travelers[index].waiting = 0;
            travelers[index].moving = 0;
            travelers[index].entering = 1;
            travelers[index].stopBeforeNextNode = 0;
            travelers[index].currentNode = msg.currentNode;
            travelers[index].nextNode = msg.nextNode;

            travelers[index].enterStartPosition = travelers[index].position;
            travelers[index].enterTargetPosition = positions[msg.currentNode];
            travelers[index].position = travelers[index].enterStartPosition;
            travelers[index].enterStartTime = GetTime();
            travelers[index].enterDuration = ENTER_ANIMATION_MS / 1000.0;

            if (msg.nextNode != -1) {
                printf("[PID=%d] arrived at node %d | next node: %d\n",
                       msg.pid,
                       msg.currentNode,
                       msg.nextNode);
                fflush(stdout);
            }

        } else if (msg.status == STATUS_FINISHED) {
            travelers[index].waiting = 0;
            travelers[index].moving = 0;
            travelers[index].entering = 0;
            travelers[index].currentNode = msg.currentNode;
            travelers[index].nextNode = -1;
            int finishedIndexOnNode = 0;
            int finishedCountOnNode = 0;

            for (int k = 0; k < travelerCount; k++) {
                if (travelers[k].finished &&
                    travelers[k].currentNode == msg.currentNode) {
                    finishedCountOnNode++;
                    }
            }

            finishedIndexOnNode = finishedCountOnNode;

            float angle = finishedIndexOnNode * 2.0f * PI / 6.0f;
            float radius = 18.0f;

            travelers[index].position.x = positions[msg.currentNode].x + cosf(angle) * radius;
            travelers[index].position.y = positions[msg.currentNode].y + sinf(angle) * radius;

            printf("[PID=%d] arrived at node %d | DESTINATION\n", msg.pid, msg.currentNode);
            printf("[PID=%d] finished\n", msg.pid);
            fflush(stdout);

            travelers[index].finished = 1;

        } else if (msg.status == STATUS_NO_PATH) {
            travelers[index].waiting = 0;
            travelers[index].moving = 0;
            travelers[index].entering = 0;

            if (msg.currentNode >= 0 && msg.currentNode < MAX_NODES) {
                travelers[index].position = positions[msg.currentNode];
            }

            printf("[PID=%d] No path found\n", msg.pid);
            printf("[PID=%d] finished\n", msg.pid);
            fflush(stdout);

            travelers[index].noPath = 1;
            travelers[index].finished = 1;
        }
    }

    for (int p = 0; p < movingCount; p++) {
        Message msg = movingMessages[p];
        int index = msg.travelerIndex;

        if (index < 0 || index >= travelerCount) continue;
        if (msg.currentNode < 0 || msg.currentNode >= MAX_NODES) continue;
        if (msg.nextNode < 0 || msg.nextNode >= MAX_NODES) continue;

        int sameTargetInBatch = 0;
        for (int q = 0; q < movingCount; q++) {
            if (movingMessages[q].nextNode == msg.nextNode) {
                sameTargetInBatch++;
            }
        }

        int targetBusy = 0;
        for (int j = 0; j < travelerCount; j++) {
            if (j == index) continue;
            if (travelers[j].finished || travelers[j].noPath) continue;

            if (travelers[j].entering && travelers[j].currentNode == msg.nextNode) {
                targetBusy = 1;
            }

            if (travelers[j].waiting && travelers[j].waitingNode == msg.nextNode) {
                targetBusy = 1;
            }

            if (travelers[j].moving && travelers[j].nextNode == msg.nextNode) {
                targetBusy = 1;
            }

            if (!travelers[j].moving &&
                !travelers[j].waiting &&
                !travelers[j].entering &&
                travelers[j].currentNode == msg.nextNode) {
                targetBusy = 1;
            }
        }

        travelers[index].waiting = 0;
        travelers[index].moving = 1;
        travelers[index].entering = 0;
        travelers[index].currentNode = msg.currentNode;
        travelers[index].nextNode = msg.nextNode;
        travelers[index].moveStartPosition = positions[msg.currentNode];

        if (sameTargetInBatch > 1 || targetBusy) {
            travelers[index].stopBeforeNextNode = 1;
            travelers[index].moveTargetPosition =
                waitingPositionOnIncomingEdge(positions[msg.currentNode],
                                              positions[msg.nextNode],
                                              index);
        } else {
            travelers[index].stopBeforeNextNode = 0;
            travelers[index].moveTargetPosition = positions[msg.nextNode];
        }

        travelers[index].position = travelers[index].moveStartPosition;
        travelers[index].moveStartTime = GetTime();

        int weight = getEdgeWeight(edges, M, msg.currentNode, msg.nextNode);

        if (weight != INF && weight > 0) {
            int travelTime = weight * EDGE_TIME_SCALE - ENTER_ANIMATION_MS;
            if (travelTime < 1) travelTime = 1;
            travelers[index].moveDuration = travelTime / 1000.0;
        } else {
            travelers[index].moveDuration = 0.3;
        }
    }
}

int shouldStopBeforeTarget(Traveler travelers[],
                           int travelerCount,
                           int travelerIndex,
                           int targetNode) {
    for (int j = 0; j < travelerCount; j++) {
        if (j == travelerIndex) continue;
        if (travelers[j].finished || travelers[j].noPath) continue;

        if (!travelers[j].moving &&
            !travelers[j].waiting &&
            travelers[j].currentNode == targetNode) {
            return 1;
        }

        if (travelers[j].waiting && travelers[j].waitingNode == targetNode) {
            return 1;
        }

        if (travelers[j].moving && travelers[j].nextNode == targetNode) {
            return 1;
        }
    }

    return 0;
}

int shouldMoveOnlyToEntrance(Traveler travelers[],
                              int travelerCount,
                              int travelerIndex,
                              int targetNode) {
    int approachingSameTarget = 0;

    for (int j = 0; j < travelerCount; j++) {
        if (travelers[j].finished || travelers[j].noPath) continue;

        if (j != travelerIndex) {
            if (!travelers[j].moving &&
                !travelers[j].waiting &&
                !travelers[j].entering &&
                travelers[j].currentNode == targetNode) {
                return 1;
            }

            if (travelers[j].entering && travelers[j].currentNode == targetNode) {
                return 1;
            }

            if (travelers[j].waiting && travelers[j].waitingNode == targetNode) {
                return 1;
            }
        }

        if (travelers[j].moving && travelers[j].nextNode == targetNode) {
            approachingSameTarget++;
        }
    }

    if (approachingSameTarget >= 2) {
        return 1;
    }

    return 0;
}


int hasNodeVisualBlocker(Traveler travelers[],
                         int travelerCount,
                         int travelerIndex,
                         int targetNode) {
    for (int j = 0; j < travelerCount; j++) {
        if (j == travelerIndex) continue;
        if (travelers[j].finished || travelers[j].noPath) continue;

        if (travelers[j].waiting && travelers[j].waitingNode == targetNode) {
            return 1;
        }

        if (travelers[j].entering && travelers[j].currentNode == targetNode) {
            return 1;
        }

        if (!travelers[j].moving &&
            !travelers[j].waiting &&
            !travelers[j].entering &&
            travelers[j].currentNode == targetNode) {
            return 1;
        }
    }

    return 0;
}

int countMoversToTarget(Traveler travelers[],
                        int travelerCount,
                        int targetNode) {
    int count = 0;

    for (int j = 0; j < travelerCount; j++) {
        if (travelers[j].finished || travelers[j].noPath) continue;

        if (travelers[j].moving &&
            !travelers[j].waiting &&
            !travelers[j].entering &&
            travelers[j].nextNode == targetNode) {
            count++;
        }
    }

    return count;
}

void resolveMovementTargets(Traveler travelers[],
                            int travelerCount,
                            Vector2 positions[]) {
    for (int i = 0; i < travelerCount; i++) {
        if (!travelers[i].moving ||
            travelers[i].waiting ||
            travelers[i].entering ||
            travelers[i].finished ||
            travelers[i].noPath ||
            travelers[i].currentNode < 0 ||
            travelers[i].currentNode >= MAX_NODES ||
            travelers[i].nextNode < 0 ||
            travelers[i].nextNode >= MAX_NODES) {
            continue;
        }

        int targetNode = travelers[i].nextNode;
        int moversToSameNode = countMoversToTarget(travelers,
                                                   travelerCount,
                                                   targetNode);
        int blocked = hasNodeVisualBlocker(travelers,
                                           travelerCount,
                                           i,
                                           targetNode);

        if (blocked || moversToSameNode > 1) {
            travelers[i].stopBeforeNextNode = 1;
            travelers[i].moveTargetPosition =
                waitingPositionOnIncomingEdge(positions[travelers[i].currentNode],
                                              positions[targetNode],
                                              i);
        } else {
            travelers[i].stopBeforeNextNode = 0;
            travelers[i].moveTargetPosition = positions[targetNode];
        }
    }
}

void updateTravelerAnimations(Traveler travelers[],
                              int travelerCount,
                              Vector2 positions[]) {
    (void)positions;
    double now = GetTime();

    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].entering &&
            !travelers[i].finished &&
            !travelers[i].noPath &&
            !travelers[i].waiting) {
            double enterElapsed = now - travelers[i].enterStartTime;
            double enterT = enterElapsed / travelers[i].enterDuration;

            if (enterT < 0.0) enterT = 0.0;
            if (enterT > 1.0) enterT = 1.0;

            travelers[i].position.x = travelers[i].enterStartPosition.x +
                                      (travelers[i].enterTargetPosition.x - travelers[i].enterStartPosition.x) * (float)enterT;
            travelers[i].position.y = travelers[i].enterStartPosition.y +
                                      (travelers[i].enterTargetPosition.y - travelers[i].enterStartPosition.y) * (float)enterT;

            if (enterT >= 1.0) {
                travelers[i].entering = 0;
                travelers[i].position = travelers[i].enterTargetPosition;
            }

            continue;
        }

        if (!travelers[i].moving ||
            travelers[i].finished ||
            travelers[i].noPath ||
            travelers[i].waiting ||
            travelers[i].nextNode < 0) {
            continue;
        }

        if (travelers[i].currentNode < 0 ||
            travelers[i].currentNode >= MAX_NODES ||
            travelers[i].nextNode < 0 ||
            travelers[i].nextNode >= MAX_NODES) {
            continue;
        }

        double elapsed = now - travelers[i].moveStartTime;
        double t = elapsed / travelers[i].moveDuration;

        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;

        Vector2 start = travelers[i].moveStartPosition;
        Vector2 movementTarget = travelers[i].moveTargetPosition;

        travelers[i].position.x = start.x + (movementTarget.x - start.x) * (float)t;
        travelers[i].position.y = start.y + (movementTarget.y - start.y) * (float)t;
    }
}

void drawTravelersPanel(Traveler travelers[], int travelerCount) {
    DrawRectangleRounded((Rectangle){690, 140, 185, 180},
                         0.15f,
                         8,
                         (Color){255, 255, 255, 225});

    DrawText("Travelers", 735, 150, 20, DARKGRAY);

    for (int i = 0; i < travelerCount; i++) {
        Color labelColor = travelers[i].color;

        const char *state = "RUNNING";

        if (travelers[i].waiting) {
            state = "WAITING";
        } else if (travelers[i].entering) {
            state = "ENTERING";
        } else if (travelers[i].moving) {
            state = "MOVING";
        } else if (travelers[i].noPath) {
            state = "NO PATH";
        } else if (travelers[i].finished) {
            state = "DONE";
        }

        DrawText(TextFormat("T%d: %d -> %d",
                            i + 1,
                            travelers[i].source,
                            travelers[i].dest),
                 710,
                 180 + i * 42,
                 16,
                 labelColor);

        DrawText(TextFormat("%s  PID=%d",
                            state,
                            travelers[i].pid),
                 710,
                 198 + i * 42,
                 14,
                 labelColor);
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
        travelers[i].waiting = 0;
        travelers[i].waitingNode = -1;
        travelers[i].moving = 0;
        travelers[i].entering = 0;
        travelers[i].stopBeforeNextNode = 0;
        travelers[i].position = positions[travelers[i].source];
        travelers[i].moveStartPosition = positions[travelers[i].source];
        travelers[i].moveTargetPosition = positions[travelers[i].source];
        travelers[i].moveStartTime = GetTime();
        travelers[i].moveDuration = 0.3;
        travelers[i].enterStartPosition = positions[travelers[i].source];
        travelers[i].enterTargetPosition = positions[travelers[i].source];
        travelers[i].enterStartTime = GetTime();
        travelers[i].enterDuration = 0.18;
        travelers[i].enterStartTime = 0.0;
        travelers[i].enterDuration = 0.18;
        travelers[i].enterStartPosition = travelers[i].position;
        travelers[i].enterTargetPosition = travelers[i].position;
    }

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Directed Weighted Graph - Stage 6");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        handleIncomingMessages(travelers, travelerCount, positions, edges, M);
        updateTravelerAnimations(travelers, travelerCount, positions);

        BeginDrawing();
        ClearBackground((Color){250, 252, 255, 255});

        drawStaticGraph(N, edges, M, positions);

        DrawText("Semaphore rule: only one traveler can stay inside each node",
                 210,
                 95,
                 18,
                 DARKGRAY);

        drawTravelersPanel(travelers, travelerCount);

        for (int i = 0; i < travelerCount; i++) {
            Color drawColor = travelers[i].color;

            DrawCircleV(travelers[i].position, 14, drawColor);

            DrawCircleLines((int)travelers[i].position.x,
                            (int)travelers[i].position.y,
                            14,
                            BLACK);

            DrawText(TextFormat("T%d", i + 1),
                     (int)travelers[i].position.x + 15,
                     (int)travelers[i].position.y - 10,
                     18,
                     drawColor);

            if (travelers[i].waiting) {
                DrawText("WAIT",
                         (int)travelers[i].position.x - 18,
                         (int)travelers[i].position.y + 20,
                         12,
                         BLACK);
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

void closeAndUnlinkSemaphores(sem_t *nodeSemaphores[],
                              char semaphoreNames[MAX_NODES][64],
                              int N) {
    for (int i = 0; i < N; i++) {
        if (nodeSemaphores[i] != SEM_FAILED) {
            sem_close(nodeSemaphores[i]);
        }

        sem_unlink(semaphoreNames[i]);
    }
}

