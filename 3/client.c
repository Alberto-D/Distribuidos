#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include "proxy.h"
#define PORT 8080
#define MAXI 256






int main(int argc, char const *argv[]){

	int thread_number;
	char client_mode;

	check_arguments_client(argc, argv, &client_mode, &thread_number);
	printf("El modo es %c y el numero es %d\n", client_mode, thread_number);

	launch_cient(thread_number);

	return 0;
}