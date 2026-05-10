CC = gcc
CFLAGS = -Wall -Wextra -std=c99

.PHONY: milestone1 milestone2 milestone3 clean

milestone1:
	$(CC) $(CFLAGS) milestone1.c -o dijkstra

milestone2:
	$(CC) $(CFLAGS) milestone2.c -o sim -lraylib -lm

milestone3:
	$(CC) $(CFLAGS) milestone3.c -o sim3 -lraylib -lm

clean:
	rm -f dijkstra sim sim3