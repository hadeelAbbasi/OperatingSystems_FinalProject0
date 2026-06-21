#ifndef GUI_H
#define GUI_H

#include "common.h"
#include "graph.h"
#include "scheduler.h"

void initNodePositions(Vector2 positions[], int N);
void initTravelerColors(Traveler travelers[], int travelerCount);
void updateTravelerAnimations(Traveler travelers[], int travelerCount);
void startMoveAnimation(Traveler *t, Vector2 from, Vector2 to, int weight);
void startEnterAnimation(Traveler *t, Vector2 from, Vector2 to);
void drawSimulation(GraphData *data,
                    Vector2 nodePositions[],
                    SchedulerType scheduler,
                    NodeQueue queues[]);

#endif