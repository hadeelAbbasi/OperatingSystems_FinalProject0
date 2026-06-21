#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "ipc.h"

void sendMessage(int fd, int travelerIndex,
                 int previousNode, int currentNode,
                 int nextNode, int remainingCost,
                 int status) {
    Message msg;

    msg.pid = getpid();
    msg.travelerIndex = travelerIndex;
    msg.previousNode = previousNode;
    msg.currentNode = currentNode;
    msg.nextNode = nextNode;
    msg.remainingCost = remainingCost;
    msg.status = status;

    write(fd, &msg, sizeof(Message));
}

int readOneMessage(int fd, Message *msg) {
    ssize_t n = read(fd, msg, sizeof(Message));

    if (n == sizeof(Message)) {
        return 1;
    }

    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }

    return 0;
}

void sleepMilliseconds(int milliseconds) {
    usleep(milliseconds * 1000);
}

void makeNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}