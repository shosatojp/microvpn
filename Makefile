client:
	gcc -g util.c client.c -o client.out
	./client.out

server:
	gcc -g util.c server.c -o server.out
	./server.out

test:
	traceroute -i tunclient 1.1.1.1 -4 -I -n