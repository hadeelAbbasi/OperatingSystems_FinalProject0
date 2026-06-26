#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <raylib.h>

#define INF 1000000000
#define MAX_NODES 15
#define MAX_EDGES 200
#define MAX_TRAVELERS 10
#define MAX_PATH 15

#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 700

#define NODE_STAY_MS 1000
#define EDGE_TIME_SCALE 1500
#define ENTER_ANIMATION_MS 40
#define SCHEDULER_GRACE_SECONDS 0.03

typedef enum {
    SCHD_FCFS = 1,
    SCHD_SJF = 2,
    SCHD_PRIORITY = 3
} SchedulerType;

typedef enum {
    MSG_REQUEST_NODE = 1,
    MSG_ARRIVED = 2,
    MSG_MOVING = 3,
    MSG_WAITING = 4,
    MSG_FINISHED = 5,
    MSG_NO_PATH = 6
} MessageStatus;

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
    int remainingCost;
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

    int childToParent[2];
    int parentToChild[2];
} Traveler;

#endif