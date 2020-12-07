
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
	enum operations	action;
};

struct response{
	    enum operations    action;
	    unsigned int  counter;
		long  waiting_time;
};



struct name{
   char name[512]; // 100 character array
};


void usage(void);

void check_arguments_client(int argc, char const *argv[], char* mode, int* threads);
void *thread_client(void *i);
void launch_cient(int thread_number);

void *thread_server(void *sockiet);
void launch_server(int eso);
void check_arguments_server(int argc, char const *argv[], int* priority);

void *thread_master(void *arg);

struct response priority_writer( struct request Peticion);
struct response priority_reader( struct request Peticion);