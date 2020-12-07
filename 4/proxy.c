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
#define Recived 1
#define Waiting 2

//Variables nedded
int sockfd =0;
int new_socket=0;
struct sockaddr_in address;
pthread_t thread;
struct sockaddr_in addresses[MAX_CLIENTS];
pthread_t threads[MAX_CLIENTS];

char name[MAXI];

int estado=Recived;
int last_ack=0;
struct send_chunk recividos[MAX_CLIENTS];

char *names[10] = {"yo", "mfernandez", "rcalvo","abanderas","pcruz"};
int number_of_names=5;
pthread_mutex_t lock;
int n =0;
char date[MAXI];


struct client list[10];


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
int close_connection(){
	pthread_join(thread,NULL);
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
			estado=Recived;
			printf("Ack %d recived form %s\n",chunk.chunk_id,chunk.username );
			if(chunk.chunk_id-last_ack <512){
				//Si el anterior ack - el nuevo ack es menor que 512 significa que se han asentido menos de 512 bytes de datos con lo cual es el ulmo trozo del mensaje.
				printf("Ultimo ack recivido\n " );
				break;
			}
		}
		last_ack=chunk.chunk_id;
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
		if(estado==Recived){
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
			estado=Waiting;
			//printf("En el bloque: NOmbre: %s, chunkid: %d, datasixe: %d, data: %s\n",tosend.username,tosend.chunk_id,tosend.data_size,tosend.data);
			return 0;
		}else if (estado==Waiting){
			sleep(0.5);
		}

		if (gettimeofday(&t2, NULL)!= 0){
			perror("Error in get time of the day");
		}
		time_taken = (t2.tv_sec - t1.tv_sec)*1000;
		time_taken += (t2.tv_usec - t1.tv_usec) / 1000.0;
		time_taken = time_taken/1000;		
	}
	printf("\nWaiting time execed, conection closed\n");
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

int get_client(char name[]){
	for(int i =0; i<10;i++){
		if((strncmp(list[i].username, name,strlen(name))==0)){
			//Si el numbre es e que estoy comparando, es un usuario que ya ha estado antes, devuelvo la i
			printf("Ya existe\n");
			list[i].is_first=0;
			return i;
		}else if(list[i].is_first){
			struct client new;
			printf("Lo creo\n");
			strcpy(new.username, name);
			new.user_id=i;
			new.number++;
			list[i]= new;
			return i;
		}
	}
}

void *thread_server(void *oldi){	 
	FILE *fptr;
  	int addrlen = sizeof(address);
	struct chunk_ack ack;

	int i = *(int*)oldi;
	struct send_chunk recivido = recividos[i];

	if(is_registred(recivido.username, names, number_of_names)){
		//Si el usuario está registrado, creo el directorio con su nombre, si ya esxiste no hago nada
		if((mkdir(recivido.username, 0700)<0)&&(errno !=EEXIST)){
			perror("mkdir failed");
			exit(EXIT_FAILURE);	
		}
		//Configuro el ack y lo envío, si falla lo digo y salgo
		strcpy(ack.username, recivido.username);
		ack.chunk_id = recivido.chunk_id;
		
		if(sendto(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&addresses[i],addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}
		//Creo las estructuras para comprobar si hanpasado 5 segundos
		
		//Si es el primer mensaje, creo el nombre
		
		pthread_mutex_lock(&lock);
		int eso = get_client(recivido.username);
		printf(" copio en filename  %s  %d-\n",list[eso].username, eso);
		//Cojo el lock y abro y escribo en el fichero, y si el mensaje es más corto que 512 bytes significa que es el último así que lo aviso y acabo.
		char filename[MAXI];
		sprintf(filename,"%s/%s.%s|%d.txt", list[eso].username,list[eso].username,date,list[eso].number);
		printf(" copio en filename  %s con number %d %d-\n",list[eso].username,list[eso].number, eso);

		if((fptr = fopen(filename,"a")) == NULL){
			printf("Error!");   
			exit(1);             
		}
		if(fprintf(fptr,"%s",recivido.data)<0){
			printf("Error escrbiendo\n");
			exit(EXIT_FAILURE);	
		}
		fclose(fptr);
		if((recivido.data_size<MAX_SIZE)&&(!list[eso].is_first)){		
			printf("\n\nSe acabo\n\n");
			list[eso].number++;
			//break;
		}
		pthread_mutex_unlock(&lock);
	}else{
		//Si el cliente es desconocido le mando un mensaje con el chunkID=-1 para indicarlo.
		fprintf(stderr,"%s :User unknown \n",recivido.username );
		strcpy(ack.username, recivido.username);
		ack.chunk_id = -1;
		if(sendto(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&addresses[i],addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}
 	  	return 0;
  	}
	  sleep(1);
  	return 0;
}

int wait_client(char *names[], int number_of_names){
  	int addrlen = sizeof(address);
	if(pthread_mutex_init(&lock, NULL) != 0){
		printf("\n mutex init failed\n");
    	return 1;
    }
	struct client zero;
	strcpy(zero.username,"" );
	zero.user_id=-1;
	zero.created=0;
	zero.is_first=1;
	zero.number=0;
	for(int i=0;i<10;i++){
		list[i]= zero;
	}

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(date,"%d-%02d-%02d_%02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);



	while(1){
		//Un while que se ejecuta eternamente, va cogiendo 100 clientes, hace 100 threads, los trata y los une.
		//Uso n para saber cuantas vueltas llevo y hacer una division clara entre una vuelta y otra.
		for(int i = 0; i < MAX_CLIENTS; i++){
			recvfrom(sockfd, &recividos[i], sizeof(recividos[i]), 0,(struct sockaddr *)&address,&addrlen);
			int *number = malloc(sizeof(*number));
			*number = i;
			addresses[i]= address;

			if(pthread_create(&threads[i], NULL, thread_server,(void *)number)!=0){
				perror("	thread error");
				exit(EXIT_FAILURE);
			}
		}

		for(int i = 0; i < MAX_CLIENTS; i++){
			if(pthread_join(threads[i], NULL)!=0){
				perror("	thread error");
				exit(EXIT_FAILURE);
			}
		}
		n++;
		printf("------------------%d------------------\n",n);
	}




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