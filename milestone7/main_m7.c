#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>

#include <raylib.h>

#include "common.h"
#include "graph.h"
#include "dijkstra.h"
#include "ipc.h"
#include "scheduler.h"
#include "gui.h"

static SchedulerType parseScheduler(const char *name) {
    if (strcmp(name, "sjf") == 0) {
        return SCHD_SJF;
    }

    return SCHD_FCFS;
}

static void printUsage(void) {
    printf("Usage:\n");
    printf("./sim -schd fcfs <file_name>\n");
    printf("./sim -schd sjf <file_name>\n");
}

static Vector2 getOutsideNodePosition(Vector2 positions[],
                                      int previousNode,
                                      int currentNode) {
    if (previousNode < 0) {
        return positions[currentNode];
    }

    Vector2 prev = positions[previousNode];
    Vector2 curr = positions[currentNode];

    Vector2 direction = {
        curr.x - prev.x,
        curr.y - prev.y
    };

    float len = sqrtf(direction.x * direction.x +
                      direction.y * direction.y);

    if (len > 0) {
        direction.x /= len;
        direction.y /= len;

        Vector2 outside = {
            curr.x - direction.x * 70,
            curr.y - direction.y * 70
        };

        return outside;
    }

    return curr;
}

static void childProcess(GraphData data, int travelerIndex) {
    Traveler *t = &data.travelers[travelerIndex];

    close(t->childToParent[0]);
    close(t->parentToChild[1]);

    int path[MAX_PATH];
    int pathLength = dijkstra(data.N,
                              data.matrix,
                              t->source,
                              t->dest,
                              path);

    if (pathLength == 0) {
        sendMessage(t->childToParent[1],
                    travelerIndex,
                    -1,
                    t->source,
                    -1,
                    INF,
                    MSG_NO_PATH);

        close(t->childToParent[1]);
        close(t->parentToChild[0]);
        exit(0);
    }

    for (int i = 0; i < pathLength; i++) {
        int previousNode = (i == 0) ? -1 : path[i - 1];
        int currentNode = path[i];
        int nextNode = (i == pathLength - 1) ? -1 : path[i + 1];

        int remainingCost = pathRemainingCost(path,
                                               pathLength,
                                               i,
                                               data.edges,
                                               data.M);

        sendMessage(t->childToParent[1],
                    travelerIndex,
                    previousNode,
                    currentNode,
                    nextNode,
                    remainingCost,
                    MSG_REQUEST_NODE);

        char permission;
        read(t->parentToChild[0], &permission, sizeof(char));

        sendMessage(t->childToParent[1],
                    travelerIndex,
                    previousNode,
                    currentNode,
                    nextNode,
                    remainingCost,
                    MSG_ARRIVED);

        sleepMilliseconds(NODE_STAY_MS);

        if (nextNode == -1) {
            sendMessage(t->childToParent[1],
                        travelerIndex,
                        previousNode,
                        currentNode,
                        nextNode,
                        0,
                        MSG_FINISHED);
            break;
        }

        sendMessage(t->childToParent[1],
                    travelerIndex,
                    currentNode,
                    currentNode,
                    nextNode,
                    remainingCost,
                    MSG_MOVING);

        int weight = getEdgeWeight(data.edges,
                                   data.M,
                                   currentNode,
                                   nextNode);

        sleepMilliseconds(weight * EDGE_TIME_SCALE);
    }

    close(t->childToParent[1]);
    close(t->parentToChild[0]);
    exit(0);
}

static void grantNextIfPossible(NodeQueue queues[],
                                GraphData *data,
                                int node,
                                SchedulerType scheduler) {
    if (node < 0 || node >= data->N) {
        return;
    }

    if (queues[node].occupied) {
        return;
    }

    if (!schedulerHasWaiting(queues, node)) {
        return;
    }

    if (queues[node].decisionTime > 0.0 &&
        GetTime() < queues[node].decisionTime) {
        return;
    }

    Message selected = schedulerPopNext(queues, node, scheduler);
    int travelerIndex = selected.travelerIndex;

    queues[node].occupied = 1;
    queues[node].decisionTime = 0.0;

    data->travelers[travelerIndex].waiting = 0;
    data->travelers[travelerIndex].waitingNode = -1;

    for (int i = 0; i < queues[node].count; i++) {
        int idx = queues[node].items[i].travelerIndex;
        data->travelers[idx].waiting = 1;
        data->travelers[idx].waitingNode = node;
    }

    char permission = 'G';
    write(data->travelers[travelerIndex].parentToChild[1],
          &permission,
          sizeof(char));
}

static void handleMessage(GraphData *data,
                          NodeQueue queues[],
                          Vector2 positions[],
                          SchedulerType scheduler,
                          Message msg,
                          int *finishedCount) {
    Traveler *t = &data->travelers[msg.travelerIndex];

    if (msg.status == MSG_REQUEST_NODE) {
        schedulerAddRequest(queues, msg);

        if (!queues[msg.currentNode].occupied &&
            queues[msg.currentNode].count == 1) {
            queues[msg.currentNode].decisionTime =
                GetTime() + SCHEDULER_GRACE_SECONDS;
        }

        t->waiting = queues[msg.currentNode].occupied ||
                     queues[msg.currentNode].count > 1;

        t->waitingNode = msg.currentNode;
        t->moving = 0;
        t->entering = 0;

        if (msg.previousNode >= 0) {
            t->position = getOutsideNodePosition(positions,
                                                 msg.previousNode,
                                                 msg.currentNode);
        }

        grantNextIfPossible(queues,
                            data,
                            msg.currentNode,
                            scheduler);
    }

    else if (msg.status == MSG_ARRIVED) {
        t->pid = msg.pid;
        t->currentNode = msg.currentNode;
        t->nextNode = msg.nextNode;
        t->waiting = 0;

        Vector2 fromPos = t->position;
Vector2 toPos = positions[msg.currentNode];

float dx = toPos.x - fromPos.x;
float dy = toPos.y - fromPos.y;
float enterDistance = sqrtf(dx * dx + dy * dy);

double duration = 0.12;

if (msg.previousNode >= 0) {
    Vector2 prev = positions[msg.previousNode];
    Vector2 curr = positions[msg.currentNode];

    float fullDx = curr.x - prev.x;
    float fullDy = curr.y - prev.y;
    float fullDistance = sqrtf(fullDx * fullDx + fullDy * fullDy);

    int w = getEdgeWeight(data->edges,
                          data->M,
                          msg.previousNode,
                          msg.currentNode);

    if (fullDistance > 0 && w != INF) {
        double fullEdgeDuration = (double)(w * EDGE_TIME_SCALE) / 1000.0;
        duration = ((double)enterDistance / (double)fullDistance) * fullEdgeDuration;
    }
}

if (duration < 0.05) {
    duration = 0.05;
}

t->entering = 1;
t->moving = 0;
t->waiting = 0;

t->enterStartPosition = fromPos;
t->enterTargetPosition = toPos;
t->enterStartTime = GetTime();
t->enterDuration = duration;

        if (msg.nextNode == -1) {
            printf("[PID=%d] arrived at node %d | DESTINATION\n",
                   msg.pid,
                   msg.currentNode);
        } else {
            printf("[PID=%d] arrived at node %d | next node: %d\n",
                   msg.pid,
                   msg.currentNode,
                   msg.nextNode);
        }
    }

    else if (msg.status == MSG_MOVING) {
        int from = msg.currentNode;
        int to = msg.nextNode;

        queues[from].occupied = 0;

        int weight = getEdgeWeight(data->edges,
                                   data->M,
                                   from,
                                   to);

        Vector2 outsideTarget = getOutsideNodePosition(positions,
                                                       from,
                                                       to);

        startMoveAnimation(t,
                           positions[from],
                           outsideTarget,
                           weight);

        grantNextIfPossible(queues,
                            data,
                            from,
                            scheduler);
    }

    else if (msg.status == MSG_FINISHED) {
        t->finished = 1;
        t->waiting = 0;
        t->moving = 0;
        t->entering = 0;
        t->position = positions[msg.currentNode];

        queues[msg.currentNode].occupied = 0;

        printf("[PID=%d] finished\n", msg.pid);

        (*finishedCount)++;

        grantNextIfPossible(queues,
                            data,
                            msg.currentNode,
                            scheduler);
    }

    else if (msg.status == MSG_NO_PATH) {
        t->noPath = 1;
        t->finished = 1;

        printf("[PID=%d] No path found\n", msg.pid);

        (*finishedCount)++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4 || strcmp(argv[1], "-schd") != 0) {
        printUsage();
        return 1;
    }

    SchedulerType scheduler = parseScheduler(argv[2]);
    const char *fileName = argv[3];

    printf("Scheduler = %s\n", schedulerName(scheduler));

    GraphData data;

    if (!readInputFile(fileName, &data)) {
        return 1;
    }

    Vector2 nodePositions[MAX_NODES];
    initNodePositions(nodePositions, data.N);
    initTravelerColors(data.travelers, data.travelerCount);

    for (int i = 0; i < data.travelerCount; i++) {
        data.travelers[i].position =
            nodePositions[data.travelers[i].source];

        if (pipe(data.travelers[i].childToParent) == -1 ||
            pipe(data.travelers[i].parentToChild) == -1) {
            printf("Pipe error\n");
            return 1;
        }
    }

    for (int i = 0; i < data.travelerCount; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            printf("Fork error\n");
            return 1;
        }

        if (pid == 0) {
            childProcess(data, i);
        }

        data.travelers[i].pid = pid;

        close(data.travelers[i].childToParent[1]);
        close(data.travelers[i].parentToChild[0]);

        makeNonBlocking(data.travelers[i].childToParent[0]);
    }

    NodeQueue queues[MAX_NODES];
    initNodeQueues(queues, data.N);

    InitWindow(SCREEN_WIDTH,
               SCREEN_HEIGHT,
               "Milestone 7 - Scheduling Algorithms");

    SetTargetFPS(60);

    int finishedCount = 0;

    while (!WindowShouldClose() &&
           finishedCount < data.travelerCount) {
        for (int i = 0; i < data.travelerCount; i++) {
            Message msg;

            while (readOneMessage(data.travelers[i].childToParent[0],
                                  &msg)) {
                handleMessage(&data,
                              queues,
                              nodePositions,
                              scheduler,
                              msg,
                              &finishedCount);
            }
        }

        for (int node = 0; node < data.N; node++) {
            grantNextIfPossible(queues, &data, node, scheduler);
        }

        updateTravelerAnimations(data.travelers,
                                 data.travelerCount);

        drawSimulation(&data,
                       nodePositions,
                       scheduler,
                       queues);
    }

    while (!WindowShouldClose()) {
        updateTravelerAnimations(data.travelers,
                                 data.travelerCount);

        drawSimulation(&data,
                       nodePositions,
                       scheduler,
                       queues);

        DrawText("All travelers finished. Press ESC to exit.",
                 260,
                 650,
                 20,
                 DARKGREEN);
    }

    CloseWindow();

    for (int i = 0; i < data.travelerCount; i++) {
        waitpid(data.travelers[i].pid, NULL, 0);

        close(data.travelers[i].childToParent[0]);
        close(data.travelers[i].parentToChild[1]);
    }

    return 0;
}