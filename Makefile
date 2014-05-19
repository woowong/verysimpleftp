target : 
	gcc -o client client.c
	gcc -o server server.c -lpthread

clean:
	rm -rf client
	rm -rf server


