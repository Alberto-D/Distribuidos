#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/select.h>
#include "proxy.h"

int main(int argc, char *argv[])
{
    int priority;
	check_arguments_server(argc, argv, &priority);
    set_ip_port("127.0.0.1", 8080);
    int socket = 0;
    init_connection_server();
    server_accept_client(socket);
}