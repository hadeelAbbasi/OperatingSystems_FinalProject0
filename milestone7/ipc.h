#ifndef IPC_H
#define IPC_H

#include "common.h"

void sendMessage(int fd, int travelerIndex,
                 int previousNode, int currentNode,
                 int nextNode, int remainingCost,
                 int status);

int readOneMessage(int fd, Message *msg);

void sleepMilliseconds(int milliseconds);

void makeNonBlocking(int fd);

#endif