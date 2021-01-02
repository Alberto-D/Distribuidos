/***********************************************************************************************
 *                                                                                             *
 *  Servidor UDP Multithread.                                                                  *
 *  Basado en https://github.com/AnkitDimri/multithread-udp-server.git                         *
 **********************************************************************************************/


#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_THREADS 128
#define MAX_USERNAME_SIZE 50
#define MAX_SIZE 512

pthread_t threads [MAX_THREADS];
int threadno = 0;
int fd;
int should_go_on;

// Datos para crear el hilo trabajador
struct thread_data {
    int thread_num;
    int init_sd;
    char username[MAX_USERNAME_SIZE];
    socklen_t addlen;
    struct sockaddr_in clientaddr;
};

int get_random(int lower, int upper);

// Gestion de interrupcions: cerramos sockets y esperamos hilos antes de salir 
void sig_handler(int signo);
 
// Hilo trabajador
void* worker_thread(void* r);

int main(int argc, char const *argv[]);

