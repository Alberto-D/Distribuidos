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


int threadno=0;


//Variables nedded for the socket connection
int sockfd =0;
int new_socket=0;
struct sockaddr_in address;
int addrlen = sizeof(address);

//Trhead for the client
pthread_t thread;
//Two arrays of 100, one of threads and the ohter of adresses, so that the server sends each packet to the correct client
struct sockaddr_in addresses[MAX_CLIENTS];
//pthread_t threads[MAX_CLIENTS];
struct sockaddr_in clientaddr;

//the name to set in setname()
char name[MAX_USERNAME_SIZE];

// An state to see if the machine is waiting fro a message, the last ack to check if it is the last message and a narray of sendchunks to use in the server
struct message recividos[MAX_CLIENTS];

//The list of registered users, its number and a list of struct client that saves various parameters of each client.
char *names[10] = {"jgines", "mfernandez", "rcalvo","abanderas","pcruz"};
int number_of_names=5;
pthread_mutex_t lock;

int n =0;

char gip[MAX_SIZE];


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

	bzero(&clientaddr, sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = inet_addr(ip);
	strcpy(gip,ip );
    clientaddr.sin_port = htons( port );
	
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
	close(sockfd);

}


//Funciones para ambos------------------------------

//Manda un primer mensaje con si es escritor o lector para empezar la comunicacacion y espera un ack
//Si llega un cormiado la conexcion está bien y se puede seguir, si no, devuelvo errro
int start_conection(struct message first){
	printf("\n\t Servidor escuchando en el puerto %u ip %s \n",clientaddr.sin_port,inet_ntoa(clientaddr.sin_addr) );

	if(sendto(sockfd, &first, sizeof(first), 0,(struct sockaddr *)&clientaddr,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	
	struct message ack;
	if(recvfrom(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&clientaddr,&addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	printf("El puerto es %d y en action %d  \n",ack.port, ack.action );
	int puerto = ack.port;
	if(ack.action==CHUNKACK){
		printf("Confirmo el mensaje\n");
		
		return puerto;
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
		printf("\n\tIntento enviar al puerto %u ip %s \n",clientaddr.sin_port,inet_ntoa(clientaddr.sin_addr) );

	//Pregunta por el chunk id y en textc se copia el texto.
	struct get_chunk chunk_to_get;
	chunk_to_get.action = GETCHUNK;
	strcpy(chunk_to_get.username, name);
	chunk_to_get.chunk_id=id;
	//Envia un getchunk pregunatndo por elchunk con id id.
	printf("MAndo un getchunk\n");
	if(sendto(sockfd, &chunk_to_get, sizeof(chunk_to_get), 0,(struct sockaddr *)&clientaddr,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	//Espera a recivir el chunk y copia el text
	struct send_chunk recived_chunk;
	if(recvfrom(sockfd, &recived_chunk, sizeof(recived_chunk), 0,(struct sockaddr *)&clientaddr,&addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	strncpy(textc,recived_chunk.data,MAX_SIZE);
	printf( "LLEga de %s con action %d \n ", recived_chunk.username, recived_chunk.action);
	//Si me llega algo que no es un send chunk fallo, igual copmprobar tambien el nombre
	if(recived_chunk.action != SENDCHUNK){
		return 1;
	}
	//Envia el asenttimiento del chunk
	
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
	
	char filename[MAX_SIZE];
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
			}	printf("\n\t Servidor escuchando en el puerto %u ip %s \n",clientaddr.sin_port,inet_ntoa(clientaddr.sin_addr) );

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





int should_go_on;

int fd;	



/* Gestion de interrupciones: cerramos socket antes de salir */
void sig_handler(int signo) {
  if (signo == SIGINT) {
    printf("\nTerminando programa...\n");
    close (fd);
    should_go_on = 0;
    for (int i = 0; i < threadno; i++) {
      pthread_join(threads[i], NULL);
    }
    printf("\nTodos los thread han terminado.\n");            
  }
}

/* Genera un entero aleatorio en el rango indicado */
int get_random(int lower, int upper) { 
  int num = (rand() % (upper - lower + 1)) + lower; 
  return num;
} 


int init_socket (int* port_to_make,struct sockaddr_in clientaddr_to_set ) {
	int port = *(int*)port_to_make;

	int local_sd;
	if ((local_sd  = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    	perror("socket failed");
    	exit(EXIT_FAILURE);
  	}else{
		printf("Socket created \n");
	}

	bzero(&clientaddr_to_set, sizeof(clientaddr_to_set));
	clientaddr_to_set.sin_family = AF_INET;
    clientaddr_to_set.sin_port = htons( port );
	clientaddr_to_set.sin_addr.s_addr = inet_addr(gip);

	if (bind(local_sd, (struct sockaddr *)&clientaddr_to_set, sizeof(clientaddr_to_set))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }else{
	 	printf("Conection created\n");
	}
		
	return local_sd;
}


 /* Hilo trabajador  */
void* worker_thread (void* r) {
	int recdata;
	int local_sd;
	int server_port;

	// Casting para leer datos del hilo
	struct thread_data rq = *( (struct thread_data*) r); 
	printf("\n Atendiendo a client [%s]  en %s : %d " , rq.username, inet_ntoa(rq.clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port));
	printf("\n Por el hilo servidor num %u \n", rq.thread_num);
	// Creamos un nuevo socket
	server_port = get_random(3000, 10000);
	struct sockaddr_in clientaddr; 
	socklen_t addrlen = sizeof(clientaddr);
	local_sd = init_socket(&server_port, clientaddr);
	printf("El numero es %d \n", server_port);
	// Se lo decimos al cliente
	struct message ack;
	ack.port= server_port;
	ack.action= CHUNKACK;
	sendto(rq.init_sd, &ack, sizeof(ack), 0, (struct sockaddr*) &rq.clientaddr, rq.addlen);
	printf("\n Tras el cambio atiendo a client [%s]  en %s : %d " , rq.username, inet_ntoa(clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port));

	char filename[MAX_USERNAME_SIZE+4];
	sprintf(filename,"%s.txt",rq.username);
	//Para que no sealicen lecturas ye scrituras a la vez se puede poner un lock a lo mejor? me parece extremo
	int fdin = open_file(filename);
	//Y en el while voy tramitando los mensajes, cada bucle es un mensaje tramitado

	// Lazo principal
	struct get_chunk chunk_to_get;
	while (should_go_on==1){
		// Limpiamos el bufer de datos
		//memset (local_buf, 0, sizeof (local_buf)); 
		
		recdata = recvfrom (local_sd, &chunk_to_get, sizeof(chunk_to_get), 0, (struct sockaddr*)  &clientaddr,  &addrlen);
		printf("Recivido un getchunk \n");
		if (recdata <= 0 )
		continue;

		struct send_chunk chunk_to_send;
		chunk_to_send.action= SENDCHUNK;
		chunk_to_send.chunk_id= chunk_to_get.chunk_id;
		chunk_to_send.data_size=100;
		strcpy(chunk_to_send.username, chunk_to_get.username);
		memset(chunk_to_send.data, 0, 512);
		int read_status = read(fdin, chunk_to_send.data, MAX_SIZE);
		if(read_status < 0){
			fprintf(stderr, "can't read");
			exit(1);
		}
		printf("\n He recibido: [%s](%d) desde %s : %d en el hilo %u" ,chunk_to_get.username , recdata, inet_ntoa (rq.clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port), rq.thread_num );         
		//strrev(local_buf);
		sendto (local_sd, &chunk_to_send, sizeof(chunk_to_send), 0, (struct sockaddr*) &rq.clientaddr, rq.addlen);
	}
	printf("Cerramos hilo (%d) ...\n", rq.thread_num);
	close (local_sd);
	return NULL;
}


int wait_client(char *names[], int number_of_names){

	int recdata;
	// direccion del cliente
	struct sockaddr_in clientaddr; 
	socklen_t addrlen = sizeof(clientaddr);
	// Buffer de datos UDP, lo uso para ver si el cliente esta registrado y sies lector o escritor
	struct message first_message;

	int port;
	// TODO: pasar como parametros
	port = 8128;
	fd = init_socket(&port, clientaddr);

	// gestion de interrupciones
	signal(SIGINT, sig_handler);
	signal(SIGTSTP, sig_handler);
	// unbuffer stdout: para ver las salidas de los hilos 
	setvbuf(stdout, NULL, _IONBF, 0); 
	should_go_on = 1;
	printf("\n\t Servidor escuchando en el puerto %u \n",port );
		
	// Lazo principal de ejecucion
	while (should_go_on==1){   
		// limpiamos buffer de datos, pniendo a -1 la action ya que es un estao en el que no puede estar.
		memset (first_message.username, 0, MAX_USERNAME_SIZE);
		first_message.action=-1;
		// Aqui esperamos nuevos clientes

		recdata = recvfrom (fd, &first_message, sizeof(first_message), 0, (struct sockaddr*) &clientaddr, &addrlen);
		if (recdata <= 0 )
		continue;




		// una vez recibida una peticion, preparamos los datos del hilo que lo atendera
		struct thread_data *r = malloc( sizeof( struct thread_data ) );  
		bzero (r, sizeof (struct thread_data));  
		r->thread_num = threadno;
		r->addlen = addrlen;
		r->clientaddr = clientaddr;
		r->init_sd = fd;
		strcpy (r->username, first_message.username);
		// Y creamos el hilo para atenderlo, que trabajara en otro hilo
		pthread_create (&threads [threadno++], NULL, worker_thread, (void*)r);
		if (threadno == MAX_THREADS){
		threadno = 0;
		printf("\n Demasiados hilos creados? ... \n");
		}
				
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