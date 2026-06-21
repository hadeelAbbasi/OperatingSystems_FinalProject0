#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

typedef struct {
    Message items[MAX_TRAVELERS * 4];
    int count;
    int occupied;
    double decisionTime;
} NodeQueue;

void initNodeQueues(NodeQueue queues[], int N);

void schedulerAddRequest(NodeQueue queues[], Message msg);

int schedulerHasWaiting(NodeQueue queues[], int node);

Message schedulerPopNext(NodeQueue queues[],
                         int node,
                         SchedulerType scheduler);

const char *schedulerName(SchedulerType scheduler);

#endif