all : receiver sender

clean :
	rm -f ./bin/*

receiver : noncanonical.c
	gcc noncanonical.c -o ./bin/receiver

sender : writenoncanonical.c
	gcc writenoncanonical.c -o ./bin/sender
