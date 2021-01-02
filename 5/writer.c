#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include "proxy.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#define PORT 8080
#define MAXI 256
#define MAX_SIZE 512






int main(int argc, char const *argv[]){
	char recived[MAX_SIZE];
	int chunck_id=0;
	struct message first_message;
	int num_chunks= argc-6;
	int counter=0;


	check_arguments_writer(argc, argv);
	set_ip_port("127.0.0.1",8000);
	set_username(argv[2]);

	init_connection_client();

	
	first_message.action=WRITER;
	strcpy(first_message.username,argv[2]);
	if (start_conection(first_message)<0){
		fprintf(stderr, "Cliente #%s - Error en la primera lectura.\n",argv[2]);
		exit(1);
	}
	printf("Conexion establecida\n");

	int fdin;
	
	//Para que no sealicen lecturas ye scrituras a la vez se puede poner un lock a lo mejor? me parece extremo
	
	fdin = open(argv[4], O_RDONLY);
	if(fdin < 0){
		err(1, "can't open input file");
	}
	int i=0;
	int a=6;

	char data[MAX_SIZE];
	
	while(i<=atoi(argv[argc-1])){
		if(read(fdin, data, MAX_SIZE) < 0){
			fprintf(stderr, "can't read");
			exit(1);
		}
		if(i==atoi(argv[a])){
			send_chunk(atoi(argv[a]), data, SENDCHUNK);
			a++;
		}
		i++;
	}
	printf(" Ultimo chunk\n");
	memset(data,0,MAX_SIZE);
	send_chunk(0, data, SYNC);
		

	close(fdin);
	return 0;
}