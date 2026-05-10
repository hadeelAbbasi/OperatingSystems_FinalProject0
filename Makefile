CC = gcc
CFLAGS = -Wall -Wextra -std=c99

.PHONY: milestone1 clean

milestone1:
	$(CC) $(CFLAGS) milestone1.c -o dijkstra

clean:
	rm -f dijkstra