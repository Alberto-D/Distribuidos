
.SUFFIXES: .o .c
.c.o:
	$(CC) -c $(CFLAGS) $<


CC = gcc
CFLAGS = -g -O2 -pthread

SRC_PROXY = proxy.h proxy.c
SRC_1 = server.c $(SRC_PROXY)
SRC_2 = reader.c $(SRC_PROXY)
SRC_3 = writer.c $(SRC_PROXY)



OBJ1 = server.o proxy.o
OBJ2 = reader.o proxy.o
OBJ3 = writer.o proxy.o



all: $(OBJ1) $(OBJ2) $(OBJ3) 
	gcc ${CFLAGS} -o server $(OBJ1)
	gcc ${CFLAGS} -o reader $(OBJ2)
	gcc ${CFLAGS} -o writer $(OBJ3)



clean:
	$(RM) $(OBJ1) $(OBJ2) $(OBJ3)  server reader writer
