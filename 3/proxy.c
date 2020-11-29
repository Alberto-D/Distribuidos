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
#define PORT 8080
#define MAXI 1024
#define MAX_CLIENTS 100
#define MAX_CHAR 20

int global_mode;


void usage(void){
	fprintf(stderr, "Client : ./client --mode writer/reader --threads number of threads\nServer : ./server --priority writer/reader\n");
	exit(1);
}
//Client functions

//Compruebo los argumentos del cliente, si hay algo mal lo digo y salgo
void check_arguments_client(int argc, char const *argv[], char* mode, int* threads){

	if(argc != 5){
		usage();
	}
	if((strcmp(argv[1], "--mode")||(strcmp(argv[3], "--threads")))){
		usage();
	}	
	if(strcmp(argv[2],"writer")==0){
		*mode = 'w';
		global_mode = WRITE;
	}else if(strcmp(argv[2],"reader")==0){
		*mode = 'r';
		global_mode = READ;
	}else{
		usage();
	}
	
	*threads = atoi(argv[4]);
	return;
}

//Realizo las conexiones pertinentes avisando y saliendo si falla algo, tambien envio el mensaje al server y espero e imprimo la respuesta
void *thread_client(void *eso){
	int newi = *(int*)eso;
	int sockfd = 0;
	struct sockaddr_in address;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Socket successfully created... \n");
	}
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons( PORT );

	if((connect(sockfd, (struct sockaddr*)&address, sizeof(address)))< 0){ 
		perror("connect failed");
        exit(EXIT_FAILURE);
    }else{
		printf("conected to the server ... \n");
	}
	
	struct request Mensaje;
	Mensaje.action = global_mode;
	

	if (send(sockfd, &Mensaje, sizeof(Mensaje), 0) < 0){
		perror("		send error");
	}

	struct response Respuesta;

	if(recv(sockfd, &Respuesta, sizeof(Respuesta), 0)<0){
		perror("		recv error");
	}

	char str_mode[10];
	if(Respuesta.action){
		strcpy(str_mode,"reader");
	}else{
		strcpy(str_mode,"writer");
	}

	printf("Cliente #%d , %s, contador=%d, tiempo=%ld ns.\n",newi, str_mode, Respuesta.counter, Respuesta.waiting_time);


	if(close(sockfd) < 0){
		perror(	"	close error");}
	return 0;
	
}
//Lanzo todos los clientes que me piden y espero a que terminen
void launch_cient(int thread_number){
	pthread_t thread[thread_number];
	int a = 0;
	for(int i=0; i<thread_number;i++){
		int *number = malloc(sizeof(*number));
		*number = i;
		if(pthread_create(&thread[i], NULL, &thread_client,(void *)number) != 0){
			perror("Thread error");
		}
	}
	for(int i=0; i<thread_number;i++){
		if ( pthread_join(thread[i], NULL)!= 0){
			perror("Error joining");
		}
	}

}




// Server functions
int priority;
unsigned int counter;
pthread_mutex_t lock;
sem_t mutex; 
sem_t mutex2;

pthread_cond_t cond;


int lectores =0;
int escritores=0;
int acabado=0;
int escritores_acabados=1;
int lectores_acabados=1;
int lectores_lock=0;
int escritores_lock=0;

//Respuesta para la prioridad de escritor
struct response priority_writer(struct request Peticion){
	FILE *fptr;
	struct timespec requestStart, requestEnd;
	unsigned int time_taken = 0;
	int printear=0;
	//Creo el puntero al archivo para escribir y los tiempos para medir
	pthread_mutex_lock(&lock);
	escritores_lock = escritores;
	pthread_mutex_unlock(&lock);
	//Utilizo escritores_lock para que no haya conflictos entre varios hilos.

	if((escritores_lock==1)&&(acabado==0)){
		//Si es el perimer escritor que entra, activo un semaforo
			printf("Proceso el primer escritor\n");
			acabado=1;
			escritores_acabados=0;
			sem_wait(&mutex2);
		}
	if(escritores_lock>0){
		//Proceso los escritores siempre que haya (escritores sea mas que 0)
		if((fptr = fopen("server_output.txt","w")) == NULL){
			printf("Error!");   
			exit(1);             
		}
		clock_gettime(CLOCK_MONOTONIC, &requestStart);
		pthread_mutex_lock(&lock);
		clock_gettime(CLOCK_MONOTONIC, &requestEnd);
		
		counter++;
		printear = counter;
		fprintf(fptr,"%d",counter);
		usleep(500000);
		escritores--;
		pthread_mutex_unlock(&lock);
		fclose(fptr);

	}
	//Vuelvo a coger escritores_lock porque escritores ha cambiado
	pthread_mutex_lock(&lock);
	escritores_lock = escritores;
	pthread_mutex_unlock(&lock);
	if((escritores_lock==0)&&(acabado==1)){
		//Si se han gestionado todos los escritores, desactivo el semaforo para que los lectores puedan procesarse
		printf("Se han procesado todos los escritores \n");
		acabado = 0;
		escritores_acabados=1;		
		sem_post(&mutex2);
	}

	if(escritores_acabados){
		//Utilizo lectores_lock para que no haya conflictos entre varios hilos.

		pthread_mutex_lock(&lock);
		lectores_lock = lectores;
		pthread_mutex_unlock(&lock);
			if(lectores_lock>0){
				//Si hay lectores que pocesar, intengo coger el semaforo (que solo estará activado si se han procesado todos los escritores)
			sem_wait(&mutex2);
			sem_post(&mutex2);
			
			clock_gettime(CLOCK_MONOTONIC, &requestStart);
			sem_wait(&mutex);
			clock_gettime(CLOCK_MONOTONIC, &requestEnd);
			printear = counter;
			usleep(500000);
			lectores--;
			sem_post(&mutex);
		}

	}
	//Creo la respuesta, la devuelvo y la envío
	time_taken = (requestEnd.tv_sec-requestStart.tv_sec)*1000000000 + (requestEnd.tv_nsec - requestStart.tv_nsec);
	struct response Respuesta;
	Respuesta.action = Peticion.action;
	Respuesta.counter = printear;
	Respuesta.waiting_time = time_taken;
	return Respuesta;
}




//Funcion igual que la anterior pero priorizando lectores
struct response priority_reader(struct request Peticion){
	FILE *fptr;
	struct timespec requestStart, requestEnd;
	unsigned int time_taken = 0;
	int printear=0;
	//Creo el puntero al archico para escribir y los tiempos para medir
	pthread_mutex_lock(&lock);
	lectores_lock = lectores;
	pthread_mutex_unlock(&lock);
	
	

	if((lectores_lock==1)&&(acabado==0)){
			printf("Poceso el primer lector\n");
			acabado=1;
			lectores_acabados=0;
			sem_wait(&mutex2);
		}
	if(lectores_lock>0){

		clock_gettime(CLOCK_MONOTONIC, &requestStart);
		sem_wait(&mutex);
		clock_gettime(CLOCK_MONOTONIC, &requestEnd);
		printear = counter;
		usleep(500000);
		lectores--;
		sem_post(&mutex);
		
	}
	pthread_mutex_lock(&lock);
	lectores_lock = lectores;
	pthread_mutex_unlock(&lock);
	if((lectores_lock==0)&&(acabado==1)){
		printf("Se han procesado todos los lectores \n");
		acabado = 0;
		lectores_acabados=1;		
		sem_post(&mutex2);
	}

	if(lectores_acabados){
		if(escritores>0){
			sem_wait(&mutex2);
			sem_post(&mutex2);
			
			if((fptr = fopen("server_output.txt","w")) == NULL){
				printf("Error!");   
				exit(1);             
			}
			clock_gettime(CLOCK_MONOTONIC, &requestStart);
			pthread_mutex_lock(&lock);
			clock_gettime(CLOCK_MONOTONIC, &requestEnd);
			
			counter++;
			printear = counter;
			fprintf(fptr,"%d",counter);
			usleep(500000);
			escritores--;
			pthread_mutex_unlock(&lock);
			fclose(fptr);
		}

	}
	printf("Lectores es : %d escritores es : %d, y primero es %d \n",lectores,escritores, acabado);
	time_taken = (requestEnd.tv_sec-requestStart.tv_sec)*1000000000 + (requestEnd.tv_nsec - requestStart.tv_nsec);
	struct response Respuesta;
	Respuesta.action = Peticion.action;
	Respuesta.counter = printear;
	Respuesta.waiting_time = time_taken;
	return Respuesta;
}







void *thread_server(void *sockiet){	 

	int new_socket = *(int*)sockiet;
	fd_set readmask;
	struct request Peticion;
	if(recv(new_socket, &Peticion, sizeof(Peticion), 0)<0){
		perror("		recv error");
	}
	//Recivo el mensaje
	
	
	//Bloqueo, si el mensaje es de lectura sumo al contador de lectura, y vicersa con la lectura, si no es ninguno de estos doy error
	if(pthread_mutex_lock(&lock)!=0){
		perror("		mutex error");
	}
		if(Peticion.action == WRITE){
			escritores++;
		}else if (Peticion.action==READ){
			lectores++;
		}else{
			printf("Error, data time unkwown\n");
			exit(1);
		}
	if(pthread_mutex_unlock(&lock)!=0){
		perror("		mutex error");
	}
	//Desbloqueo para que otros puedan acceder (Lo de arriab esta mal tabulado pero es porque me parece que así queda más claro)
	struct response Respuesta;

	
	if(priority==WRITE){
		Respuesta = priority_writer(Peticion);
	}
	else if(priority==READ){
		Respuesta = priority_reader(Peticion);
	}else{
		printf("Error, data time unkwown\n");
		exit(1);
	}

	if (send(new_socket, &Respuesta, sizeof(Respuesta), 0) < 0){
		perror("		send error");
	}
		printf("Accion :  %d , counter : %d, waiting = %ld  \n", Respuesta.action, Respuesta.counter, Respuesta.waiting_time);	
  	return 0;
}


//Lanzo el servidor e inicializo el mutex y los semaforos
void launch_server(int prioriy_to_set){
	priority = prioriy_to_set;
	int server_fd;
    struct sockaddr_in address;
	
	int addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Socket successfully created... \n");
	}
	//Creo la ip y el puerto
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons( PORT );
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Bind successfully created... \n");
	}

	if (listen(server_fd, 10) < 0){
		 perror("listen failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Server listening... \n");
	}

	int n = 0;

	if(pthread_mutex_init(&lock, NULL) != 0){
		printf("\n mutex init failed\n");
    	return;
    }
	
	int new_socket[MAX_CLIENTS];
	pthread_t thread[MAX_CLIENTS];
	if(sem_init(&mutex, 0, MAX_CLIENTS)!=0){
		perror("	sem error");
		exit(EXIT_FAILURE);
	}
	if(sem_init(&mutex2, 0, 1)!=0){
		perror("	sem error");
		exit(EXIT_FAILURE);
	}
	

	while(1){
		//Un while que se ejecuta eternamente, va cogiendo 100 clientes, hace 100 threads, los trata y los une.
		//Uso n para saber cuantas vueltas llevo y hacer una division clara entre una vuelta y otra
		
		
	
		for(int i = 0; i < MAX_CLIENTS; i++){
			new_socket[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
			if (new_socket[i]<0){
			perror("	accept error");
			exit(EXIT_FAILURE);
			}
			if(pthread_create(&thread[i], NULL, &thread_server, &new_socket[i])!=0){
				perror("	thread error");
				exit(EXIT_FAILURE);
			}
		}

		for(int i = 0; i < MAX_CLIENTS; i++){
			if(pthread_join(thread[i], NULL)!=0){
				perror("	thread error");
				exit(EXIT_FAILURE);
			}
		}
		n++;
		printf("------------------%d------------------\n",n);
	}
	pthread_mutex_destroy(&lock);
	close(server_fd);
	return;
}


//Comprueba los argumentos dels erver
void check_arguments_server(int argc, char const *argv[], int* priority){

	if(argc != 3){
		usage();
	}
	if((strcmp(argv[1], "--priority"))){
		usage();
	}	
	if(strcmp(argv[2],"writer")==0){
		*priority = WRITE;
	}else if(strcmp(argv[2],"reader")==0){
		*priority = READ;
	}else{
		usage();
	}

	return;
}