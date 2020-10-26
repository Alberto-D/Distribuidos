#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT 1234
#define MAXI 256


int main(int argc, char const *argv[])
{
	int sockfd=0, connfd =0, bew_socket=0;
	char buffer[1024] = {0};
	struct sockaddr_in address;
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
	char senbdbuff[MAXI];
	char msg[MAXI];
	int n;
	while(1){

		printf(">");
		fgets (senbdbuff, MAXI, stdin);
		//strncat( senbdbuff, msg, sizeof(msg));
		send(sockfd, senbdbuff, strlen(senbdbuff), 0);
		bzero(senbdbuff, MAXI);

		recv(sockfd, (void*) buffer, sizeof(buffer), MSG_DONTWAIT);
			printf(" +++ %s", buffer);
			bzero(buffer, MAXI);


	}
	close(sockfd);
	return 0;
}
