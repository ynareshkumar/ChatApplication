all:newserver client
newserver: newserver.o sbcpformat.h
	gcc -Wall -o newserver newserver.o 
client: client.o sbcpformat.h
	gcc -Wall -o client client.o
clean:
	rm -f *.o
	rm -f newserver client
