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


//Variables nedded for the socket connection
int sockfd =0;
int new_socket=0;
struct sockaddr_in address;
	int addrlen = sizeof(address);

//Trhead for the client
pthread_t thread;
//Two arrays of 100, one of threads and the ohter of adresses, so that the server sends each packet to the correct client
struct sockaddr_in addresses[MAX_CLIENTS];
pthread_t threads[MAX_CLIENTS];

//the name to set in setname()
char name[MAXI];

// An state to see if the machine is waiting fro a message, the last ack to check if it is the last message and a narray of sendchunks to use in the server
struct message recividos[MAX_CLIENTS];

//The list of registered users, its number and a list of struct client that saves various parameters of each client.
char *names[10] = {"jgines", "mfernandez", "rcalvo","abanderas","pcruz"};
int number_of_names=5;
pthread_mutex_t lock;

int n =0;
//The current date goes here, so that two files never have the same name.
char date[MAXI];



//Common functions---------------------------------------------------------------------
void usage(void){
	fprintf(stderr, "./client --username #name --chunks #N\nServer : ./server --priority writer/reader\n");
	exit(1);
}

void set_username (const char* username){
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
int close_connection(){
	pthread_join(thread,NULL);
	//close(sockfd);

}


//Funciones para ambos------------------------------

//Manda un primer mensaje con si es escritor o lector para empexzar la comunicacacion y espera un ack
//Si llega un cormiado la conexcion está bien y se puede seguir, si no, devuelvo errro
int start_conection(struct message first){
	if(sendto(sockfd, &first, sizeof(first), 0,(struct sockaddr *)&address,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	struct message ack;
	if(recvfrom(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&address,&addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}

	if(ack.action==CONFIRM){
		printf("Confirmo el mensaje\n");
		return 1;
	}
	return -1;
}

//Writer---------------------------------
int send_chunk(int id, char textc[],enum actions action){
	struct send_chunk chunk_to_send;

	chunk_to_send.action= action;

	strcpy(chunk_to_send.username, name);
	chunk_to_send.chunk_id=id;
	
	strncpy(chunk_to_send.data, textc, MAX_SIZE);

	if(sendto(sockfd, &chunk_to_send, sizeof(chunk_to_send), 0,(struct sockaddr *)&address,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	wait_ack(address);
	
}








//Reader functions---------------------------------------------------------------------
int ask_for_chunk(char textc[],int id){
	//Pregunta por el chunk id y en textc se copia el texto.
	struct get_chunk chunk_to_get;
	chunk_to_get.action = GETCHUNK;
	strcpy(chunk_to_get.username, name);
	chunk_to_get.chunk_id=id;
	//Envia un getchunk pregunatndo por elchunk con id id.
	if(sendto(sockfd, &chunk_to_get, sizeof(chunk_to_get), 0,(struct sockaddr *)&address,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	//Espera a recivir el chunk y copia el text
	struct send_chunk recived_chunk;
	if(recvfrom(sockfd, &recived_chunk, sizeof(recived_chunk), 0,(struct sockaddr *)&address,&addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	strncpy(textc,recived_chunk.data,MAX_SIZE);
	//Si me llega algo que no es un send chunk fallo, igual copmprobar tambien el nombre
	if(recived_chunk.action != SENDCHUNK){
		return 1;
	}
	//Envia el asenttimiento del chunk
	struct chunk_ack asentimiento;
	asentimiento.chunk_id=id;
	if(sendto(sockfd, &asentimiento, sizeof(asentimiento), 0,(struct sockaddr *)&address,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	return 0;
}


//Client functions---------------------------------------------------------------------
void check_arguments_reader(int argc, char const *argv[]){
	if(argc != 5){
		usage();
	}else if((strcmp(argv[1], "--username")||(strcmp(argv[3], "--chunks")))){
		usage();

	}
	return;
}

void check_arguments_writer(int argc, char const *argv[]){
	if((strcmp(argv[1], "--username")||(strcmp(argv[3], "--path"))||(strcmp(argv[5], "--chunks")) )){
		usage();
	}
	//Compruebo si los numeros estan ordenados de menor a mayor, 7e es el priemr numero en el array
	for(int i=7; i<argc;i++){
		if(atoi(argv[i])<=(atoi(argv[i-1]))){
			usage();
		}
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

int wait_ack(struct sockaddr_in address){
	struct chunk_ack asentimiento;
			if(recvfrom(sockfd, &asentimiento, sizeof(asentimiento), 0,(struct sockaddr *)&address,&addrlen)<0){
				perror("Failed sendto");
				exit(EXIT_FAILURE);
			}

printf("Recivo el ack de %s, shunkid %d action %d\n",asentimiento.username, asentimiento.chunk_id, asentimiento.action );
}




void *thread_server_reader(void *oldi){
	int i = *(int*)oldi;
printf("	INICIO EL THREAD  %d, LECTOR\n",i);
	struct message recivido = recividos[i];
	struct sockaddr_in address = addresses[i];
	//Compruebo que el usuario está registrado con un primer paso de mensajes para ver si continuar o no la comunicacion.

	struct message enviar;
	enviar.action=DENNY;
	strcpy(enviar.username, recivido.username);

	if(is_registred(recivido.username, names, number_of_names)){
		enviar.action= CONFIRM;
	}
	if(sendto(sockfd, &enviar, sizeof(enviar), 0,(struct sockaddr *)&addresses[i],addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	//Tras esto, abro el archivo a leer 
	
	char filename[MAXI];
	sprintf(filename,"%s.txt",recivido.username);
	//Para que no sealicen lecturas ye scrituras a la vez se puede poner un lock a lo mejor? me parece extremo
	int fdin = open_file(filename);
	//Y en el while voy tramitando los mensajes, cada bucle es un mensaje tramitado
	while(1){
		//Recivo un getchunk con el chunk que quiere el cliente
		struct get_chunk chunk_to_get;
		if(recvfrom(sockfd, &chunk_to_get, sizeof(chunk_to_get), 0,(struct sockaddr *)&address,&addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}
		//Creo un sendchunk para enviarlo.
		struct send_chunk chunk_to_send;
		chunk_to_send.action= SENDCHUNK;
		//Si es el ultimo mensaje, en el id irá un -1, envio un ultimo mensaje, espero al ack y termino el bucle.
		if(chunk_to_get.chunk_id==-1){
			if(sendto(sockfd, &chunk_to_send, sizeof(chunk_to_send), 0,(struct sockaddr *)&address,addrlen)<0){
				perror("Failed sendto");
				exit(EXIT_FAILURE);
			}
			wait_ack(address);
			break;
		}
		//Relleno el chunk con la informacion necesaria y lo envio
		chunk_to_send.chunk_id= chunk_to_get.chunk_id;
		chunk_to_send.data_size=100;
		strcpy(chunk_to_send.username, chunk_to_get.username);
		memset(chunk_to_send.data, 0, 512);
		int read_status = read(fdin, chunk_to_send.data, MAX_SIZE);
		if(read_status < 0){
			fprintf(stderr, "can't read");
			exit(1);
		}
		if(sendto(sockfd, &chunk_to_send, sizeof(chunk_to_send), 0,(struct sockaddr *)&address,addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}
		//Finalmente espero al ack del mensaje
		wait_ack(address);

	}
	close(fdin);
	printf("ACAbo el while de mandar cosas\n");
  	return 0;
}


void *thread_server_writer(void *oldi){
	int i = *(int*)oldi;
printf("	INICIO EL THREAD  %d, ESCRITOR\n",i);
	struct message recivido = recividos[i];
	//Compruebo que el usuario está registrado con un primer paso de mensajes para ver si continuar o no la comunicacion.
	struct message enviar;
	int  fptr;
	enviar.action=DENNY;
	strcpy(enviar.username, recivido.username);

	if(is_registred(recivido.username, names, number_of_names)){
		enviar.action= CONFIRM;
		
	}

	if(sendto(sockfd, &enviar, sizeof(enviar), 0,(struct sockaddr *)&addresses[i],addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	//Abro el archivo temporal, si existe  lo sobre escribo
	if((fptr = open("temp.txt", O_WRONLY | O_CREAT, 0777))<0){
				perror("Failed open");
				exit(EXIT_FAILURE);            
			}
	//HAsta que no me llege un mensaje sync recivo y proceso mensajes, escribiendo los chunks en el archivo temporal.
	while (1){
		struct send_chunk recived;
		if(recvfrom(sockfd, &recived, sizeof(recived), 0,(struct sockaddr *)&address,&addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}
		//Que copie en el texto y poner un semaforo para el lector
		if(recived.action==SENDCHUNK){	
			if(write(fptr,recived.data, strlen(recived.data))<0){
				printf("Writing failled\n");
				exit(EXIT_FAILURE);	
			}
		}else if(recived.action==SYNC){
			struct chunk_ack asentimiento;
			asentimiento.action=CHUNKACK;
			asentimiento.chunk_id=recived.chunk_id;
			strcpy(asentimiento.username,recived.username);
			if(sendto(sockfd, &asentimiento, sizeof(asentimiento), 0,(struct sockaddr *)&address,addrlen)<0){
				perror("Failed sendto");
					exit(EXIT_FAILURE);
			}
			break;
		}
		
		struct chunk_ack asentimiento;
		asentimiento.action=CHUNKACK;
		asentimiento.chunk_id=recived.chunk_id;
		strcpy(asentimiento.username,recived.username);
		if(sendto(sockfd, &asentimiento, sizeof(asentimiento), 0,(struct sockaddr *)&address,addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}

	}
	//Una vez he acabado cierro el archivo y aviso.
	close(fptr);
	printf("SAlgo del while\n\n \n");
	
}






int wait_client(char *names[], int number_of_names){
  	int addrlen = sizeof(address);
	if(pthread_mutex_init(&lock, NULL) != 0){
		printf("\n mutex init failed\n");
    	return 1;
    }	

	while(1){	
		for(int i = 0; i < MAX_CLIENTS; i++){
			recvfrom(sockfd, &recividos[i], sizeof(recividos[i]), 0,(struct sockaddr *)&address,&addrlen);
			int *number = malloc(sizeof(*number));
			*number = i;
			addresses[i]= address;
			switch (recividos[i].action){
				case READER:
					if(pthread_create(&threads[i], NULL, thread_server_reader,(void *)number)!=0){
						perror("	thread error");
						exit(EXIT_FAILURE);
					}	
				break;
				
				case WRITER:
					printf("Esctirotr\n");
					if(pthread_create(&threads[i], NULL, thread_server_writer,(void *)number)!=0){
						perror("	thread error");
						exit(EXIT_FAILURE);
					}	
				break;

				default:

				break;
			}
			if(pthread_join(threads[i], NULL)!=0){
				perror("	thread error");
				exit(EXIT_FAILURE);
			}
		}

		
		//I use n to know how many times the bucle have been executed
		n++;
		printf("------------------%d------------------\n",n);
	}

  	return 0;   
}

int is_registred(char username[],char *strs[], int size){
	int exit=0;
	for(int i =0;i< size ;i++){
		if(strncmp(strs[i], username,strlen(username))==0){
			exit=1;
		}
	}
	return exit;
}