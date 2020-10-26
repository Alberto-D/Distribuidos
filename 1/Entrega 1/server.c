#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT 8080
#define MAXI 1024
#define MAX_CLIENTS 100

void *thread_client(void *socket){	 

	int new_socket = *(int*)socket;
	char buffer[1024] = {0};
	fd_set readmask;
	struct timeval timeout;
	char senbdbuff[MAXI];
	//Connfiguro los parametros para usar select, si falla aviso.
	//No pongo un if en estas funciones porque el codigo se complicaría demasiado.
	FD_ZERO(&readmask); // Reset la mascara.
	FD_SET(STDIN_FILENO, &readmask);
	FD_SET(new_socket, &readmask);
	timeout.tv_sec=0; timeout.tv_usec = 2000000; // Timeout.
	if (select(new_socket+1, &readmask, NULL, NULL, &timeout) <0){
		perror("	Select error");
	}
	// Si hay datos que leer del descriptor, los leo, si falla aviso, si no, lo imprimo.
	if (FD_ISSET(new_socket, &readmask))
	{
		if (recv(new_socket, (void*) buffer, sizeof(buffer), MSG_DONTWAIT)<0){
			perror("		Recv error");}
		printf("+++ %s", buffer);
		bzero(buffer, MAXI);
	}
	//Envio el hello client al cliente y cierro el socket, si algo falla aviso.
	strcpy(senbdbuff, "Hello client!");
	if((send(new_socket, senbdbuff, strlen(senbdbuff), 0))<0){
		perror("		send error");
	}
	bzero(senbdbuff, MAXI);
	if(close(new_socket) < 0){
		perror(	"	close error");
	}
	return 0;
}

int main(int argc, char const *argv[])
{

	//Configuro lo necesario para usar sockets, si algo falla aviso.
    int server_fd;
    struct sockaddr_in address;
	
	int addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
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
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("binf failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Bind successfully created... \n");
	}

	if (listen(server_fd, 10) < 0)
	{
		 perror("listen failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Server listening... \n");
	}

	int n = 0;
	while(1){
		//Un while que se ejecuta eternamente, va cogiendo 100 clientes, hace 100 threads, los trata y los une.
		//Uso n para saber cuantas vueltas llevo y hacer una division clara entre una vuelta y otra
		int new_socket[MAX_CLIENTS];
		pthread_t thread[MAX_CLIENTS];
	
		for(int i = 0; i < MAX_CLIENTS; i++){
			new_socket[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
			if (new_socket[i]<0)
			{
			perror("	accept error");
			exit(EXIT_FAILURE);
			}
			pthread_create(&thread[i], NULL, &thread_client, &new_socket[i]);
		}

		for(int i = 0; i < MAX_CLIENTS; i++){
			pthread_join(thread[i], NULL);
		}
		n++;
		printf("------------------%d------------------\n",n);
	}
	close(server_fd);
    return 0;
}



