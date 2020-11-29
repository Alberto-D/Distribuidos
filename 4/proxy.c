#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <pthread.h>
#include <sys/select.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include "proxy.h"
#include  <unistd.h>
#include <fcntl.h>
#include <err.h>

#define PORT 8128
#define MAXI 1024
#define MAX_CLIENTS 100
#define MAX_CHAR 20
#define MAX_SIZE 512

int sockfd =0;
int new_socket=0;
struct sockaddr_in address;
pthread_t thread;

char name[MAXI];

//Common functions---------------------------------------------------------------------
void usage(void){
	fprintf(stderr, "./client --username #name --path #path\nServer : ./server --priority writer/reader\n");
	exit(1);
}


void set_username (char* username){
	strcpy(name, username);
	printf("Username is %s\n",name);

}



void set_ip_port (char* ip, unsigned int port) {

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    	perror("socket failed");
    	exit(EXIT_FAILURE);
  	}else{
		printf("Socket created \n");
	}

	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons( port );
}


int open_file(char * strFileName){
	
	int fdin = open(strFileName, O_RDONLY);
	if(fdin < 0){
		err(1, "can't open input file");
	}
	return fdin;


}

int close_file(int fd){
	close(fd);

}


//Client functions---------------------------------------------------------------------

void check_arguments_client(int argc, char const *argv[]){
	if(argc != 5){
		usage();
	}else if((strcmp(argv[1], "--username")||(strcmp(argv[3], "--path")))){
		usage();

	}else if( access( argv[4], F_OK ) == -1 ){
		usage();
	}
	
	return;
}


int init_connection_client() {

	if((connect(sockfd, (struct sockaddr*)&address, sizeof(address)))< 0){
		perror("conection failed");
	}else{
    	printf("Conection created\n");
	}
	return sockfd; 
}

void init_recv_thread () {
//Inicio el hilo que va a estar escuchando..
  pthread_create(&thread, NULL, thread_reception, NULL);
}

void *thread_reception(void *unused){
	struct chunk_ack chunk;
 	recv(sockfd, &chunk, sizeof(chunk), 0);
  	return 0;
}

int write_block(int fdin, char * strData, int byteOffset, int blockSize){
	struct send_chunk tosend;
	memset(tosend.data, 0, MAX_SIZE);
	strcpy(tosend.username, name);
	tosend.chunk_id=byteOffset;
	tosend.data_size=blockSize;
	printf("\n---------------------------------\n");
	strcpy(tosend.data, strData);
	send(sockfd, &tosend, sizeof(tosend), 0);
	printf("En el bloque: NOmbre: %s, chunkid: %d, datasixe: %d, data: %s\n",tosend.username,tosend.chunk_id,tosend.data_size,tosend.data);
	// char buff[MAX_SIZE];
	// int read_status;
	// int done;
	// done = 0;	
	// while(!done){
	// 	read_status = read(fdin, tosend.data, MAX_SIZE);
	// 	if(read_status < 0){
	// 		printf("can't read");
	// 		return -1;
	// 	}else if(read_status == 0){
	// 		done = 1;
	// 		break;
	// 	}
	// 	printf("%s",tosend.data);
	//  	printf("\n---------------------------------\n");
	// 	//strcpy(tosend.data, buff);
	// 	send(sockfd, &tosend, sizeof(tosend), 0);
	// 	memset(tosend.data, 0, MAX_SIZE);
	// }
	return 0;

}


//Server functions---------------------------------------------------------------------

int init_connection_server() {

   if (bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

	if (listen(sockfd, 1) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
  return 0; 
}


int wait_client(char *names[], int number_of_names){

 	struct send_chunk recivido;
  	int addrlen = sizeof(address);

  	if ((new_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
		perror("accept");
		exit(EXIT_FAILURE);
	}
	
	recv(new_socket, &recivido, sizeof(recivido), 0);
	printf("En el bloque: NOmbre: %s, chunkid: %d, datasixe: %d, data: %s\n",recivido.username,recivido.chunk_id,recivido.data_size,recivido.data);


  	if(is_registred(recivido.username, names, number_of_names)!=0){
  	  fprintf(stderr,"User unknown \n");
 	  return 1;
  	}
	  int eso =1;
	  while (eso>0){
		  	memset(recivido.data, 0, MAX_SIZE);

		eso = recv(new_socket, &recivido, sizeof(recivido), 0);
		printf("En el bloque: NOmbre: %s, chunkid: %d, datasixe: %d, data: %s\n",recivido.username,recivido.chunk_id,recivido.data_size,recivido.data);

	  }
  	return 0;   
}

int is_registred(char username[],char *strs[], int size){
	int exit=1;
	for(int i =0;i< size ;i++){
		//printf("Comparo %s y %s en %d\n",strs[i], username, strlen(strs[i]));
		if(strncmp(strs[i], username,strlen(strs[i]))==0){
			exit=0;
		}
	}
	return exit;
}