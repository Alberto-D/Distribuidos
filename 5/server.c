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
	//Configuro lo necesario para usar sockets, si algo falla aviso.
	char *names[10] = {"yo", "mfernandez", "rcalvo","abanderas","pcruz"};
	set_ip_port("127.0.0.1",8000);
	init_connection_server();

	wait_client(names, 5);

	// int eso =is_registred("jginess",strs, 5);
	// if (eso==0){
	// 	printf("aaaaaaaaaaaaaaaaa\n");

	// }
    return 0;
}



