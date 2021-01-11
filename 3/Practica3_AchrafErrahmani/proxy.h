#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>


enum operations {
    WRITE = 0,
    READ
};

struct request{
    enum operations action;
};

struct response{
    enum operations action;
    unsigned int counter;
    long waiting_time;
};


void usage(void);
void check_arguments_client(int argc, const char  *argv[], char* mode, int* threads);

void check_arguments_server(int argc,  const char *argv[], int* priority);

void set_ip_port (char* ip, unsigned int port);
int init_connection_client();


int isString(char *numero);
void send_clients_and_close(int nClients, char *mode);
void server_accept_client(int sockfd);
int connect_to_server();
