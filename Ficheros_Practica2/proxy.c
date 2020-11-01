#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/param.h>

#include "proxy.h"

#define MAXI 1024
#define MAX_CLIENTS 2

struct message Menji;
int sockfd=0, i=0;
int new_socket[MAX_CLIENTS];
struct sockaddr_in address;
int addrlen = sizeof(address);
pthread_t thread;

char first[20];
char second[20];




void *thread_reception(void *unused){
  struct message recivido;

  recv(sockfd, &recivido, sizeof(recivido), 0);
	printf("Recivido en tread: %s,%d,%d\n", recivido.origin,recivido.action,recivido.clock_lamport);
  Menji.clock_lamport = MAX(recivido.clock_lamport, Menji.clock_lamport)+1;
}


// *** Funciones comunes ***
void set_name (char name[2]) {
  
  memset(Menji.origin, 0, sizeof(Menji.origin)); 
  strcpy(Menji.origin, name);
  Menji.clock_lamport = 0;
  printf("El nombre de setname es %s \n",Menji.origin);
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
  return Menji.clock_lamport;  
}

int close_connection() {
  pthread_join(thread,NULL);
  close(sockfd);

  return 0;
}


// *** Funciones para el cliente (P1,P3) ***
void init_recv_thread () {
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
  Menji.action = READY_TO_SHUTDOWN;
  Menji.clock_lamport++;
  printf(" El mensaje en notify ready es de %s, lleva %d, y el clock es %d\n", Menji.origin, Menji.action, Menji.clock_lamport);

  send(sockfd,&Menji, sizeof(Menji), 0);
    
}


void notify_shutdown_ack() {

  Menji.action = SHUTDOWN_ACK;
  printf(" El mensaje en ack es de %s, lleva %d, y el clock es %d\n", Menji.origin, Menji.action, Menji.clock_lamport);
   Menji.clock_lamport++;//poner el MAX 
  send(sockfd, &Menji, sizeof(Menji), 0); 
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
  if ((new_socket[i] = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	recv(new_socket[i], &recivido, sizeof(recivido), 0);
	printf("En wait %s,%d,%d\n", recivido.origin,recivido.action,recivido.clock_lamport);
  Menji.clock_lamport = MAX(Menji.clock_lamport, recivido.clock_lamport)+1;
	
  //send(new_socket[i], "perfe\n", 40, 0);
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
  enviar.clock_lamport = Menji.clock_lamport+1;
  int number;
  if(strcmp (clientid, first)==0){
    printf("Son el mismo priemro weeeeee\n");
    number = 0;
  }else if( strcmp (clientid, second)==0){
    number = 1;
    printf("Son el mismo SEGUNDO weeeeee\n");
  }
  printf(" El mensaje en now es de %s, lleva %d, y el clock es %d\n", enviar.origin, enviar.action, enviar.clock_lamport);
  send(new_socket[number],&enviar, sizeof(enviar), 0);//Tengo que ver como eliegir por cualmandarlo
  struct message recivido;

  recv(new_socket[number], &recivido, sizeof(recivido), 0);
  Menji.clock_lamport = MAX(Menji.clock_lamport, recivido.clock_lamport)+1;


  return 0;
}





