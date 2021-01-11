
.SUFFIXES: .o .c
.c.o:
	$(CC) -c $(CFLAGS) $<


CC = gcc
CFLAGS = -g -O2 -pthread -Wall -Wshadow

SRC_PROXY = proxy.h proxy.c
SRC_1 = server.c $(SRC_PROXY)
SRC_2 = client.c $(SRC_PROXY)

OBJ1 = server.o proxy.o
OBJ2 = client.o proxy.o

all:  $(server) $(client)
all: $(OBJ1) $(OBJ2)
	gcc ${CFLAGS} -o server $(OBJ1)
	gcc ${CFLAGS} -o client $(OBJ2)


clean:
	$(RM) $(OBJ1) $(OBJ2) server client
