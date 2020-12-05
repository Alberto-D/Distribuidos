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
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>


#define PORT 8128
#define MAXI 1024
#define MAX_CLIENTS 100
#define MAX_CHAR 20
#define MAX_SIZE 512
#define Recivido 1
#define Esperando 2


int sockfd =0;
int new_socket=0;
struct sockaddr_in address;
pthread_t thread;

char name[MAXI];

int estado=Recivido;
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

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
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
	int addrlen = sizeof(address);
	//siempre estoy reciviendo cosas, si me llega algo pongo el estado a recivido 
	while(1){
		int eso =recvfrom(sockfd, &chunk, sizeof(chunk), 0,(struct sockaddr *)&address,&addrlen);
		if(eso>0){
			estado=Recivido;
		}
	}
	
  	return 0;
}

int write_block(int fdin, char * strData, int byteOffset, int blockSize){
	struct timeval t1, t2;
	long double time_taken = 0.0;
	if (gettimeofday(&t1, NULL)!= 0){
		perror("Error in get time of the day");
	}
	while(time_taken < 5){
		if(estado==Recivido){
			//Si el estado es recivido, significa que he recivodo el ack con lo cual mando otro
			struct send_chunk tosend;
			int addrlen = sizeof(address);

			memset(tosend.data, 0, MAX_SIZE);
			strcpy(tosend.username, name);
			tosend.chunk_id=byteOffset;
			tosend.data_size=blockSize;
			//printf("\n---------------------------------\n");
			strncpy(tosend.data, strData,blockSize);
			if(sendto(sockfd, &tosend, sizeof(tosend), 0,(struct sockaddr *)&address,addrlen)<0){
				perror("Failed sendto");
				exit(EXIT_FAILURE);
			}
			estado=Esperando;
			printf("En el bloque: NOmbre: %s, chunkid: %d, datasixe: %d, data: %s\n",tosend.username,tosend.chunk_id,tosend.data_size,tosend.data);
			return 0;
		}else if (estado==Esperando){
			//Si el estado es esperando, no he recivido el ack, imprimo
			printf("a");
			sleep(0.5);
		}

		if (gettimeofday(&t2, NULL)!= 0){
			perror("Error in get time of the day");
		}
		time_taken = (t2.tv_sec - t1.tv_sec)*1000;
		time_taken += (t2.tv_usec - t1.tv_usec) / 1000.0;
		time_taken = time_taken/1000;		
	}
	printf("\nSE ACABO\n");
	exit(EXIT_FAILURE);	
}


//Server functions---------------------------------------------------------------------

int init_connection_server() {
   if (bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }else{
	 	printf("Conection created\n");
	}
  return 0; 
}


int wait_client(char *names[], int number_of_names){
	FILE *fptr;
 	struct send_chunk recivido;
  	int addrlen = sizeof(address);
	struct chunk_ack ack;
  	
	int eso =recvfrom(sockfd, &recivido, sizeof(recivido), 0,(struct sockaddr *)&address,&addrlen);
	if(eso<0){
		perror("recvfrom failed");
        exit(EXIT_FAILURE);	
	}
  	if(is_registred(recivido.username, names, number_of_names)){
		if((mkdir(recivido.username, 0700)<0)&&(errno !=EEXIST)){
			perror("mkdir failed");
			exit(EXIT_FAILURE);	
		}
		// memset(ack.username, 0, MAX_SIZE);
		 strcpy(ack.username, recivido.username);
		ack.chunk_id = recivido.chunk_id;
		
			if(sendto(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&address,addrlen)<0){
				perror("Failed sendto");
				exit(EXIT_FAILURE);
			}
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		char filename[MAXI];
		sprintf(filename,"%s/%d-%02d-%02d %02d:%02d:%02d\n",recivido.username, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		printf(" %s\n--------------------\n",recivido.data);
		if((fptr = fopen(filename,"w")) == NULL){
			printf("Error!");   
			exit(1);             
		}
		if(fprintf(fptr,"%s",recivido.data)<0){
			printf("Error escrbiendo\n");
			exit(EXIT_FAILURE);	
		}
		
		while (eso>0){
			memset(recivido.data, 0, MAX_SIZE);
			eso = recvfrom(sockfd, &recivido, sizeof(recivido), 0,(struct sockaddr*)&address,&addrlen);
			if(fprintf(fptr,"%s",recivido.data)<0){
				printf("Writing error\n");
			}
			//sleep(6);
			
			printf(" %s\n--------------------\n",recivido.data);				
			strcpy(ack.username, recivido.username);
			ack.chunk_id = recivido.chunk_id;
			if(sendto(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&address,addrlen)<0){
				perror("Failed sendto");
				exit(EXIT_FAILURE);
			}
			if(recivido.data_size<510){
				break;
			}

		}
	}else{
		fprintf(stderr,"User unknown \n");
		exit(EXIT_FAILURE);	
 	  	return 1;
  	}
	fclose(fptr);
  	return 0;   
}

int is_registred(char username[],char *strs[], int size){
	int exit=0;
	for(int i =0;i< size ;i++){
		//printf("Comparo %s y %s en %d\n",strs[i], username, strlen(strs[i]));
		if(strncmp(strs[i], username,strlen(username))==0){
			exit=1;
		}
	}
	return exit;
}