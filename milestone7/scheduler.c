#include <stdio.h>
#include "scheduler.h"

void initNodeQueues(NodeQueue queues[], int N) {
    for (int i = 0; i < N; i++) {
        queues[i].count = 0;
        queues[i].occupied = 0;
        queues[i].decisionTime = 0.0;
    }
}

void schedulerAddRequest(NodeQueue queues[], Message msg) {
    int node = msg.currentNode;

    if (node < 0 || node >= MAX_NODES) {
        return;
    }

    if (queues[node].count >= MAX_TRAVELERS * 4) {
        return;
    }

    queues[node].items[queues[node].count++] = msg;

    printf("[SCHEDULER] T%d waits for node %d | remainingCost=%d\n",
           msg.travelerIndex + 1,
           node,
           msg.remainingCost);
}

int schedulerHasWaiting(NodeQueue queues[], int node) {
    return node >= 0 &&
           node < MAX_NODES &&
           queues[node].count > 0;
}

Message schedulerPopNext(NodeQueue queues[],
                         int node,
                         SchedulerType scheduler) {
    int chosen = 0;

    if (scheduler == SCHD_SJF) {
        for (int i = 1; i < queues[node].count; i++) {
            if (queues[node].items[i].remainingCost <
                queues[node].items[chosen].remainingCost) {
                chosen = i;
            }
        }
    }

    Message result = queues[node].items[chosen];

    printf("[SCHEDULER] selected T%d for node %d using %s | remainingCost=%d\n",
           result.travelerIndex + 1,
           node,
           schedulerName(scheduler),
           result.remainingCost);

    for (int i = chosen; i < queues[node].count - 1; i++) {
        queues[node].items[i] = queues[node].items[i + 1];
    }

    queues[node].count--;

    return result;
}

const char *schedulerName(SchedulerType scheduler) {
    if (scheduler == SCHD_SJF) {
        return "SJF";
    }

    return "FCFS";
}