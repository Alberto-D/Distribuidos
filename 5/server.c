#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "proxy.h"
#define PORT 8128
#define MAXI 1024

unsigned int counter;
long waiting_time;

int main(int argc, char const *argv[])
{
	int priority;
	//Configuro lo necesario para usar sockets, si algo falla aviso.
	char *names[10] = {"jgines", "mfernandez", "rcalvo","abanderas","pcruz"};
	set_ip_port("127.0.0.1",8128);
	//Espero a los clientes.
	wait_client(names, 5);

    return 0;
}



