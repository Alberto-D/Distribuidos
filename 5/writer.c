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

#define PORT 8128
#define MAXI 256
#define MAX_SIZE 512

int main(int argc, char const *argv[]){
	char recived[MAX_SIZE];
	int chunck_id=0;
	struct message first_message;
	int num_chunks= argc-6;
	int counter=0;

	int eso = get_client_id("abanderas");
	printf( " LA id es %d \n", eso);
	//Compruebo argumentos e inicio la conexion.
	check_arguments_writer(argc, argv);
	set_ip_port("127.0.0.1",8128);
	set_username(argv[2]);
 
	init_connection_client();
	first_message.action=WRITER;
	strcpy(first_message.username,argv[2]);
	int port;
	if ((port= start_conection(first_message))<0){
		fprintf(stderr, "Cliente #%s - Error en la primera lectura.\n",argv[2]);
		exit(1);
	}
	//Cierro la primera conexion y abro otra en el puerto indicado.
	close_connection();
	printf("Cilente #%s puerto %d \n",argv[2], port);
	set_ip_port("127.0.0.1",port);
	init_connection_client();
	// Abro el archivo del que voy a leer.
	int fdin;	
	fdin = open(argv[4], O_RDONLY);
	if(fdin < 0){
		err(1, "can't open input file");
	}
	int i=0;
	int a=6;

	char data[MAX_SIZE];
	//Voy comprbando si el chunk que he leido hay que enviarlo para hacerlo o no.
	while(i<=atoi(argv[argc-1])){
		if(read(fdin, data, MAX_SIZE) < 0){
			fprintf(stderr, "can't read");
			exit(1);
		}
		if(i==atoi(argv[a])){
			int chunk_ack = send_chunk(atoi(argv[a]), data, SENDCHUNK);
			if(chunk_ack==DECISION){
				close(fdin);
				printf("Acabando programa escritor... \n");
				return 0;
			}
			a++;
		}
		i++;
	}
	printf("Enviando ultimo chunk\n");
	memset(data,0,MAX_SIZE);
	send_sync(0, data, SYNC);
	printf("Acabando programa escritor ... \n");

	close(fdin);
	close_connection();

	return 0;
}