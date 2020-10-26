#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#define PORT 8080
#define MAXI 256


int main(int argc, char const *argv[])
{
	//Configuro lo necesario para usar sockets, si algo falla aviso.
	int sockfd = 0;
	char buffer[1024] = {0};
	struct sockaddr_in address;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
        perror("socket failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Socket successfully created... \n");
	}
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons( PORT );

	if((connect(sockfd, (struct sockaddr*)&address, sizeof(address)))< 0)
	{ 
		perror("connect failed");
        exit(EXIT_FAILURE);
    }else{
		printf("conected to the server ... \n");
	}

	sleep(1);
	char senbdbuff[MAXI];
	if (argc != 2)
	{
		fprintf(stderr,"need an id\n");
		exit(EXIT_FAILURE);
	}

	snprintf(senbdbuff, MAXI, " Hello server! From client %s\n",argv[1] );

	if (send(sockfd, senbdbuff, strlen(senbdbuff), 0) < 0)
	{
		perror("		send error");
	}
	bzero(senbdbuff, MAXI);

	if(recv(sockfd, (void*) buffer, sizeof(buffer), 0)<0)
	{
		perror("		recv error");
	}
	printf("+++ %s\n", buffer);
	bzero(buffer, MAXI);
	
	if(close(sockfd) < 0){
		perror(	"	close error");}
	return 0;
}