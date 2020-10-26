#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT 1234
#define MAXI 1024

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
	char buffer[1024] = {0};
	int addrlen = sizeof(address);

	fd_set readmask;
	struct timeval timeout;


	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Socket created \n");
	}
	//creamos ip y puerto
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons( PORT );
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

	if (listen(server_fd, 10) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}


	char senbdbuff[MAXI];
	int n = 1 ;

	int status;

	while(n>0){

		printf(">");
		fgets (buffer, MAXI, stdin);
		send(new_socket, buffer, strlen(buffer), 0);
		bzero(buffer, MAXI);

		FD_ZERO(&readmask); // Reset la mascara
		FD_SET(STDIN_FILENO, &readmask);
		FD_SET(new_socket, &readmask);
		timeout.tv_sec=0; timeout.tv_usec = 3000000; // Timeout de 3 seg.
		status = select(new_socket+1, &readmask, NULL, NULL, &timeout);

		if (FD_ISSET(new_socket, &readmask))
		{// Hay datos que leer del descriptor}
			int n = recv(new_socket, (void*) buffer, sizeof(buffer), 0);
			if (n == 0){
				close(new_socket);
				return 0;
			}
			printf("+++ %s", buffer);
			bzero(buffer, MAXI);
		}
	}



	// 	printf(">");
	// 	fgets (senbdbuff, MAXI, stdin);
	// 	send(new_socket, senbdbuff, strlen(senbdbuff), 0);
	// 	bzero(senbdbuff, MAXI);


	// 	int n = recv(new_socket, (void*) buffer, sizeof(buffer),MSG_DONTWAIT);
	// 	printf("+++ %s", buffer);
	// 	bzero(buffer, MAXI);

	// }
	close(server_fd);
    return 0;
}



