#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/param.h>

#include "proxy.h"

#define MAXI 1024
#define MAX_CLIENTS 2
#define MAX_CHAR 20

// Creo un mensaje para usar el relog de lanport y tener un mensaje base.
struct message Mensaje;
// Creo el socket, la i que usaré como indice en el array new_socket (done estan los accept de los clientes),tambien el thread para usarlo.
int sockfd=0, i=0;
int new_socket[MAX_CLIENTS];
struct sockaddr_in address;
pthread_t thread;

char first[MAX_CHAR];
char second[MAX_CHAR];

//El thread del cliente recive un mansaje, si no el SHUTDOWN_NOW doy error y vuelvo.
void *thread_reception(void *unused){
  struct message recivido;
  recv(sockfd, &recivido, sizeof(recivido), 0);
  if( recivido.action != SHUTDOWN_NOW){
    fprintf(stderr,"SHUTDOWN_NOW expected but recived other message \n");
    return 0;
  }
    Mensaje.clock_lamport = MAX(recivido.clock_lamport, Mensaje.clock_lamport)+1;
  return 0;
}


// *** Funciones comunes ***
void set_name (char name[2]) {

  memset(Mensaje.origin, 0, sizeof(Mensaje.origin)); 
  strcpy(Mensaje.origin, name);
  Mensaje.clock_lamport = 0;
}

void set_ip_port (char* ip, unsigned int port) {

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
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


int get_clock_lamport(){

  return Mensaje.clock_lamport;  
}

int close_connection() {
//Uno los thread (si lo hace el servidor, el join da error pero no afecta al programa) y cierro el socket.
  pthread_join(thread,NULL);
  close(sockfd);
  return 0;
}


// *** Funciones para el cliente (P1,P3) ***
void init_recv_thread () {
//Inicio el hilo que va a estar escuchando..
  pthread_create(&thread, NULL, thread_reception, NULL);
}


int init_connection_client() {

  if((connect(sockfd, (struct sockaddr*)&address, sizeof(address)))< 0)
	{
		perror("conection failed");
	}else{
    printf("Conection created\n");
  }
  return 0; 
}

void notify_ready_shutdown() {

  Mensaje.action = READY_TO_SHUTDOWN;
  Mensaje.clock_lamport++;
  send(sockfd,&Mensaje, sizeof(Mensaje), 0);
}

void notify_shutdown_ack() {

  Mensaje.action = SHUTDOWN_ACK;
   Mensaje.clock_lamport++;//poner el MAX 
  send(sockfd, &Mensaje, sizeof(Mensaje), 0); 
}

// *** Funciones para el servidor (P2) ***
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

int wait_shutdown_notication() {

  struct message recivido;
  int addrlen = sizeof(address);

  if ((new_socket[i] = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	recv(new_socket[i], &recivido, sizeof(recivido), 0);
  if( recivido.action != READY_TO_SHUTDOWN){
    fprintf(stderr,"READY_TO_SHUTDOWN expected but recived other message \n");
    return 1;
  }
  Mensaje.clock_lamport = MAX(Mensaje.clock_lamport, recivido.clock_lamport)+1;
	  if(i==0){
    strcpy(first,recivido.origin);
  }else{
    strcpy(second,recivido.origin);
  }
  i++;
  return 0;   
}

int shutdown_now(char clientid[2]) {

  struct message enviar;
  strcpy(enviar.origin, clientid);
  enviar.action = SHUTDOWN_NOW;
  enviar.clock_lamport = Mensaje.clock_lamport+1;
  int number;
  if(strcmp (clientid, first)==0){
    number = 0;
  }else if( strcmp (clientid, second)==0){
    number = 1;
  }
  send(new_socket[number],&enviar, sizeof(enviar), 0);//Tengo que ver como eliegir por cualmandarlo
  struct message recivido;

  recv(new_socket[number], &recivido, sizeof(recivido), 0);
  if( recivido.action != SHUTDOWN_ACK){
    fprintf(stderr,"SHUTDOWN_ACK expected but recived other message \n");
    return 1;
  }
  Mensaje.clock_lamport = MAX(Mensaje.clock_lamport, recivido.clock_lamport)+1;
  return 0;
}





