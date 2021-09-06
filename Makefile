#Makefile for CS270 Project 4 - simpsh
#Luke Williams

CFLAGS = -Wall -g

run: simpsh
	./simpsh
	
simpsh: simpsh.c csapp.c
	gcc $(CFLAGS) -o simpsh simpsh.c csapp.c

tar:
	tar czf proj4.tgz ltwi229
