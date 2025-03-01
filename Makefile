PORT=57510
CFLAGS = -g -Wall -Werror -std=gnu99


friend_server: friend_server.c friends.o friends.h
	gcc -DPORT=$(PORT) ${CFLAGS} -o friend_server friends.o friend_server.c

friendme: friendme.o friends.o
	gcc $(CFLAGS) -o friendme friendme.o friends.o

friendme.o: friendme.c friends.h
	gcc $(CFLAGS) -c friendme.c

friends.o: friends.c friends.h
	gcc $(CFLAGS) -c friends.c

clean:
	rm friendme friend_server *.o