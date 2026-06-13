CC = gcc
CFLAGS = -Wall -Wextra -std=c99

.PHONY: milestone1 milestone2 milestone3 milestone4 milestone5 milestone6 clean

milestone1:
	$(CC) $(CFLAGS) milestone1.c -o dijkstra

milestone2:
	$(CC) $(CFLAGS) milestone2.c -o sim -lraylib -lm

milestone3:
	$(CC) $(CFLAGS) milestone3.c -o sim -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

milestone4:
	$(CC) $(CFLAGS) milestone4.c -o sim -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

milestone5:
	$(CC) $(CFLAGS) milestone5.c -o sim -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

milestone6:
	$(CC) $(CFLAGS) milestone6.c -o sim -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

clean:
	rm -f dijkstra sim sim3