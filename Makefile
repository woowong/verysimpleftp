target : 
	gcc -o client client.o
	gcc -o server server.o

client.o:
	gcc -c client.c

server.o:
	gcc -c server.c

clean:
	rm -rf *.o
	rm -rf client
	rm -rf server


