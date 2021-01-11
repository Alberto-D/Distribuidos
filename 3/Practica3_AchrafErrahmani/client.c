#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "proxy.h"

int main(int argc, char *argv[])
{
    int thread_number;
	char client_mode;

	check_arguments_client(argc, argv, &client_mode, &thread_number);
	printf("El modo es %c y el numero es %d\n", client_mode, thread_number);

    set_ip_port("127.0.0.1", 8080);
    init_connection_client();
    int threads = 0;
    threads = strtol(argv[4], NULL, 0);
    //connect_to_server();
    send_clients_and_close(threads, argv[2]);
    
    exit(0);
}