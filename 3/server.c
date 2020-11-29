#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "proxy.h"
#define PORT 8080
#define MAXI 1024
#define MAX_CLIENTS 100

unsigned int counter;
long waiting_time;

int main(int argc, char const *argv[])
{
	int priority;
	check_arguments_server(argc, argv, &priority);
	//Configuro lo necesario para usar sockets, si algo falla aviso.
	launch_server(priority);
    return 0;
}



