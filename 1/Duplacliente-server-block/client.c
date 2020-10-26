#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT 1234
#define MAXI 256


int main(int argc, char const *argv[])
{
	int sockfd=0, connfd =0, bew_socket=0;
	//char buffer[1024] = {0};
	struct sockaddr_in address;

	fd_set readmask;
	struct timeval timeout;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
        perror("socket failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Socket created \n");
	}

	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons( PORT );

	if((connect(sockfd, (struct sockaddr*)&address, sizeof(address)))< 0)
	{
		perror("conection failed");
	}
	sleep(1);
	char buffer[MAXI];
	char msg[MAXI];
	int n;

	int status;
	while(1){

		printf(">");
		fgets (buffer, MAXI, stdin);
		send(sockfd, buffer, strlen(buffer), 0);
		bzero(buffer, MAXI);

		FD_ZERO(&readmask); // Reset la mascara
		FD_SET(STDIN_FILENO, &readmask);
		FD_SET(sockfd, &readmask);
		timeout.tv_sec=0; timeout.tv_usec = 3000000; // Timeout de 3 seg.
		status = select(sockfd+1, &readmask, NULL, NULL, &timeout);

		if (FD_ISSET(sockfd, &readmask))
		{// Hay datos que leer del descriptor}
			int n = recv(sockfd, (void*) buffer, sizeof(buffer), 0);
			if (n == 0){
				close(sockfd);
				return 0;
			}
			printf("+++ %s", buffer);
			bzero(buffer, MAXI);
		}
	}
	close(sockfd);
	return 0;
}
