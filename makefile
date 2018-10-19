#########################################################
# Makefile for CAB403 Systems Programming Project	
# Author: Marcus van Egmond (n9937439)			
#							
# Produces: ./client & ./server				
#########################################################

CC = gcc
CFLAGS = -Wall

all: server client
	rm -f *.o
	@echo Compilation finished!

client: ms_client.o utils.o
	$(CC) $(CFLAGS) -o client ms_client.o utils.o
server: ms_server.o utils.o ms.o
	$(CC) $(CFLAGS) -o server ms_server.o utils.o ms.o
	
