#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include "proxy.h"
#define PORT 8128
#define MAXI 256
#define MAX_SIZE 512
#define MAX_TEXT 1060260


int main(int argc, char const *argv[]){

	char text[MAX_TEXT];
	char recived[MAX_SIZE];
	int chunck_id=0;
	struct message first_message;
	int num_chunks= atoi(argv[4]);
	int counter=0;


	check_arguments_reader(argc, argv);
	set_ip_port("127.0.0.1",8128);
	set_username(argv[2]);

	init_connection_client();
	first_message.action=READER;
	strcpy(first_message.username,argv[2]);
	int port;
	if ((port= start_conection(first_message))<0){
		fprintf(stderr, "Cliente #%s - Error en la primera lectura.\n",argv[2]);
		exit(1);
	}

	close_connection();
	printf("Cilente #%s puerto %d \n",argv[2], port);
	set_ip_port("127.0.0.1",port);
	init_connection_client();

	while (num_chunks >counter){
		memset(recived,0,MAX_SIZE);
		if(ask_for_chunk(recived, counter)!=0){
			fprintf(stderr,"Cliente #%s - Error en la  lectura.\n",argv[2]);
			exit(1);
		}
		counter++;
		//Podría imprimir poco a poco el texto, pero segun he entendido el enunciado hay que guardarlo e imprimirlo al final asi que eso hago.
		strncat(text, recived, MAX_SIZE);
	
	}
	 //Como ya ha acabado la comunicacion envio un mensaje con id -1 para indicar que el servidor puede dejar de escuchar.
	if(ask_for_chunk(recived, -1)!=0){
		fprintf(stderr,"Cliente #%s - Error en la ultima lectura.\n",argv[2]);
		exit(1);
	}
	printf("El texto es : %s \n",text);

	close_connection();
	return 0;
}