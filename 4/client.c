#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include "proxy.h"
#define PORT 8080
#define MAXI 256
#define MAX_SIZE 512






int main(int argc, char const *argv[]){
	check_arguments_client(argc, argv);
	set_username(argv[2]);

	set_ip_port("127.0.0.1",8000);
	//Para usar maquinas distintas hay que poner la direccion inet del server 
	
	init_connection_client();

	// Esta función debe crear un thread the recepción de mensajes y volver
	// inmediatamente la ejecución a este proceso.
	int fichero =  open_file(argv[4]);

	//write_block(fichero, "Hola k tal", 2, 3);
	init_recv_thread();

	char buff[MAX_SIZE+1];
	int read_status;
	int done;
	done = 0;	
	while(!done){
		read_status = read(fichero, buff, MAX_SIZE);
		if(read_status < 0){
			printf("can't read");
			return -1;
		}else if(read_status == 0){
			done = 1;
			break;
		}
		//printf("\n---------------------------------\n");
		
		write_block(fichero, buff, 0, read_status);
	 	

		memset(buff, 0, MAX_SIZE);
	}
	
	



	return 0;
}