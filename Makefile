CC=gcc
singerc: singerc.c 
	$(CC) -Og singerc.c -o singerc -lm -lasound -lpthread
sing1: sing1.c 
	$(CC) -Og sing1.c -o sing1 -lX11 -lm -lasound -lpthread
all: singerc sing1
