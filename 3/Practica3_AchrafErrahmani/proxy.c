#define _GNU_SOURCE
#include "proxy.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <err.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#define NSEC_PER_SEC 1000000000
#define MAX 256
#define SLEEP_50_MS 50000


int sockfd = 0;
struct sockaddr_in address;




int  contador = 0, num_wr = 0, num_rd = 0, ratio = -1;
char mode_c[10], priority[20], IP[20];
sem_t  sem_reader, sem_ratio;
int count = 100, rd = 0, wr = 0, PORT;
pthread_mutex_t mutex_s;
pthread_mutex_t mutex_c;
pthread_cond_t cond_wr, cond_rd;



struct client_info{
    char mode[10];
    int tid;
    struct request req;

};

struct attend{
    int connfd;
    struct request req_a;
};


enum{
    MAX_THREADS = 10000,
    MAX_CLIENTS = 100,
    MAX_READERS = 100,
    FILE_SIZE = 128,
};





int countDigits( int value ){
    int result = 0;
    while( value != 0 ) {
       value /= 10;
       result++;
    }
    return result;
}


void * attend_wr(void * args){

    struct timespec start, end;
    struct response resp;
    
    int fd = -1;
    char cont[1024];

    
    if(clock_gettime(CLOCK_MONOTONIC, &start) == -1)
        errx(EXIT_FAILURE, "clock gettime failed");
    
    pthread_mutex_lock(&mutex_s);

    struct attend *att = (struct attend *)args;

    if(clock_gettime(CLOCK_MONOTONIC, &end) == -1)
        errx(EXIT_FAILURE, "clock gettime failed");
    


    resp.action = att->req_a.action;
    resp.waiting_time = ((long)(end.tv_sec-start.tv_sec)*NSEC_PER_SEC + 
    (long)(end.tv_nsec-start.tv_nsec));
    contador++;
    
    if(usleep(SLEEP_50_MS) == -1)
        errx(EXIT_FAILURE, "usleep failed");

    resp.counter = contador;
    sprintf(cont, "%d", contador);
    
    if((fd = open("server_output.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
        fprintf(stderr, "error: can't open file\n");
    
    if(write(fd, cont, countDigits(contador)) < 0)
        fprintf(stderr, "error: can't write file\n");


    if (send(att->connfd, &resp, sizeof(struct response), 0) < 0)
        fprintf(stderr, "error: can't send response from writer\n");

    printf("Escritor modifica el contador con valor %d\n", contador);

    
    free(args);
    
    wr++;
    if (strcmp(priority, "reader") == 0){
        while(num_rd > 0 && (rd%ratio != 0 || ratio == -1)){
            pthread_cond_wait(&cond_wr, &mutex_s);
        }

    }else{
        if (ratio != -1){
            if (wr % ratio == 0 && num_rd>0){
                pthread_cond_signal(&cond_rd);
                wr = 0;
            }
        }
    }
    

    if (ratio != -1){
        if (rd%ratio == 0 && num_rd > 0){
            if(sem_post(&sem_ratio)==-1)
                errx(EXIT_FAILURE, "signal failed");
        }
    }    
    if(close(fd)==-1)
        errx(EXIT_FAILURE, "close failed");
    pthread_mutex_unlock(&mutex_s);
    return NULL;
}



void *attend_rd(void * args){
    
    struct response resp;
    struct timespec start, end;

    
    if(clock_gettime(CLOCK_MONOTONIC, &start) == -1)
        errx(EXIT_FAILURE, "clock gettime failed");
    
    if(sem_wait(&sem_reader) == -1)
        errx(EXIT_FAILURE, "sem_wait failed");

    struct attend *att = (struct attend *)args;

    if(clock_gettime(CLOCK_MONOTONIC, &end) == -1)
        errx(EXIT_FAILURE, "clock gettime failed");

    resp.action = att->req_a.action;
    resp.waiting_time = ((long)(end.tv_sec-start.tv_sec)*NSEC_PER_SEC + 
    (long)(end.tv_nsec-start.tv_nsec));
    

    pthread_mutex_lock(&mutex_c);
    
    rd++;
    if (strcmp(priority, "writer") == 0){
        while(num_wr>0 && (wr%ratio != 0 || ratio == -1)){
            pthread_cond_wait(&cond_rd, &mutex_c);
        }
    }else{
        if (ratio != -1){
            if (rd % ratio == 0 && num_wr > 0){
                if(sem_wait(&sem_ratio) == -1)
                    errx(EXIT_FAILURE, "sem wait failed");
                pthread_cond_signal(&cond_wr);
                rd = 0;
                
            }
        }
    }
    resp.counter = contador;
    pthread_mutex_unlock(&mutex_c);
    
    // sleep 50ms
    if(usleep(SLEEP_50_MS) == -1)
        errx(EXIT_FAILURE, "usleep failed");
    
    // envio al cliente la respuesta
    if (send(att->connfd, &resp, sizeof(struct response), 0) < 0)
        fprintf(stderr, "error: can't send response from reader\n");

    free(args);

    // signal par dejar pasar a otro thread
    if(sem_post(&sem_reader) == -1)
        errx(EXIT_FAILURE, "signal failed");
    
    
    return NULL;
}


void *socketThread(int *arg){
    struct request req;
    int connfd = *arg;   
    free(arg);
    struct attend *att;
    int n=0;
    int r = 0;
    pthread_t thread_wr[MAX_THREADS];
    pthread_t threads_rd[MAX_THREADS];
    
    //-------------- Recibimos mensaje del cliente -----------------------------
    if(sem_init(&sem_reader,0,MAX_READERS) == -1)
        errx(EXIT_FAILURE, "sem init failed");
    

    while(recv(connfd, &req, sizeof(struct request), 0) > 0){
        att = malloc(sizeof(struct attend));
        att->connfd = connfd;
        att->req_a = req;
        if (req.action == WRITE){
            num_wr++;
            if (pthread_create(&thread_wr[n], NULL, attend_wr, att) != 0)
                errx(EXIT_FAILURE, "failed creating writer thread\n");
            n++;

            // Liberamos recursos cada 100 escritores y dejamos paso al siguiente cliente(si hay alguno esperando)
            if (num_wr == 100){
                n = 0;
                for(int i = 0; i < num_wr; i++){
                    if (pthread_join(thread_wr[n], NULL) != 0)
                        fprintf(stderr, "failed joining writer thread\n");
                    n++;
                }
                n=0;
                num_wr = 0;
            }
        }else{
            num_rd++;
            if (pthread_create(&threads_rd[r], NULL, attend_rd, att) != 0)
                errx(EXIT_FAILURE, "failed creating reader thread\n");
            r++;

            // Liberamos recursos cada 100 lectores
            if (num_rd == 100){
                r = 0;
                for(int i = 0; i < num_rd; i++){
                    if (pthread_join(threads_rd[r], NULL) != 0)
                        fprintf(stderr, "failed joining reader thread\n");
                    r++;
                }
                r=0;
                num_rd = 0;
            }
        }  
    }


    if (req.action == WRITE)
        num_wr = 0;
    else if (req.action == READ)
        num_rd = 0;

    if (num_wr == 0 && ratio != -1){
        pthread_cond_broadcast(&cond_rd);
        if (ratio != -1)
            sem_post(&sem_ratio);
    }
    
    if (num_rd == 0 && ratio != -1)
        pthread_cond_broadcast(&cond_wr);
    
    
    //---------------------------------------------------------------------------

    printf("Exit client\n");
    //cerramos conexion con el cliente
    if(close(connfd) == -1)
        errx(EXIT_FAILURE, "cant close connfd");
    
    return NULL;
}









// -----------------Checking Errors---------------------------------------------



int isString(char *numero){
    int i = 0;
    int error = 0;
    while(numero[i] != '\0' && !error){
        if (numero[i] < '0' || numero[i] > '9'){
            error=1;
        }
        i++;
    }
    return error;
}

void usage(void){
	fprintf(stderr, "Client : ./client --mode writer/reader --threads number of threads\n");
    fprintf(stderr, "Server : ./server --priority writer/reader\n");
	exit(1);
}


//Compruebo los argumentos del cliente, si hay algo mal lo digo y salgo
void check_arguments_client(int argc,  const char *argv[], char* mode, int* threads){
	if(argc != 5){
		usage();
	}
	if((strcmp(argv[1], "--mode")||(strcmp(argv[3], "--threads")))){
		usage();
	}	
	if(strcmp(argv[2],"writer")==0){
		*mode = 'w';
	}else if(strcmp(argv[2],"reader")==0){
		*mode = 'r';
	}else{
		usage();
	}
	*threads = atoi(argv[4]);
	return;
}


//Comprueba los argumentos del server
void check_arguments_server(int argc, char const *argv[], int* priority_s){
	if((argc != 3)&&(argc != 5)){
		usage();
	}
	if((strcmp(argv[1], "--priority"))){
		usage();
	}	
	if(strcmp(argv[2],"writer")==0){
		*priority_s = WRITE;
	}else if(strcmp(argv[2],"reader")==0){
		*priority_s = READ;
	}else{
		usage();
	}
    if(argc==5){
        if((strcmp(argv[3], "--ratio")))
		    usage();
    }
	return;
}

// ---------------------------------------------------------------------------------


void server_accept_client(int sockfd2){
    int n=0, connfd,t=0;

    pthread_t threads[MAX_CLIENTS];

    while (1){
        if (t == 0){
            printf("\nWaiting for connection...\n\n");
            t++;
        }


        //----------- Se establece conexión con cada cliente--------------------

        connfd = accept(sockfd, (struct sockaddr*) NULL, NULL);
        if (connfd < 0)
            errx(EXIT_FAILURE, "Server accept failed...");
        else
            printf("Server accepts the client...\n");

        //-----------------------------------------------------------------------

        int *pclient = malloc(sizeof(int));
        *pclient = connfd;

        //------- Creamos los threads -------------------------------------------

        if(pthread_create(&threads[n], NULL, (void *)socketThread, pclient) < 0)
            errx(EXIT_FAILURE, "error creating thread");
        else
            n++;

        //------------------------------------------------------------------------
        
        if(n == MAX_CLIENTS) {    
            n = 0;
            while(n < MAX_CLIENTS){ // Terminar con todos los threads{
                if (pthread_join(threads[n], NULL) < 0)
                    errx(EXIT_FAILURE, "can't join thread");

                n++;
            }
            n=0;
            t=0;
        }
    }
    if(close(sockfd) == -1)
        errx(EXIT_FAILURE, "cant close sockfd");
}

//Conection fase -------------------------

void set_ip_port (char* ip, unsigned int port){
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }else{
		printf("Socket created \n");
	}

	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons( port );
}

int init_connection_client(){
    if((connect(sockfd, (struct sockaddr*)&address, sizeof(address)))< 0){
		perror("conection failed");
	}else{
        printf("Conection created\n");
    }
  return 0; 
}


int init_connection_server(){
   if (bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

	if (listen(sockfd, 1) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}
  return 0; 
}
 

int connect_to_server(){

    // pthread_mutex_lock(&mutex_c);
    struct sockaddr_in servaddr;

    // Creamos un socket TCP y comprobamos que se ha creado correctamente
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        errx(EXIT_FAILURE, "Socket creation failed...");
    else
        printf("Socket successfully created...\n");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(IP);
    servaddr.sin_port = htons(PORT);

    // Conectamos el cliente al socket del servidor y comprobamos su estado
    int conect = 0;
    if ((conect = (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0))
        errx(EXIT_FAILURE,"Connection with the server failed...");
    else
        printf("Connected to the server...\n");

    
    return sockfd;
}


char * mode_client(int mode_i){
    switch (mode_i){
        case WRITE:
            snprintf(mode_c, sizeof(mode_c), "WRITE");
            break;
        case READ:
            snprintf(mode_c, sizeof(mode_c), "READ");
            break;
    }
    return mode_c;
}

void *clientFunction(void *arg){

    struct client_info *ci = (struct client_info *)arg;
    struct response resp_client;
    if (strcmp(ci->mode, "reader") == 0)
        ci->req.action = READ;
    else if (strcmp(ci->mode, "writer") == 0)
        ci->req.action = WRITE;

        
    if(send(sockfd, &ci->req, sizeof(struct request), 0) < 0)
        fprintf(stderr, "send failed\n");

    if(recv(sockfd, &resp_client, sizeof(struct response), 0) > 0){
        printf("Client: %d  %s, CONTADOR: %d, WAITING_TIME: %ldns\n", ci->tid,
        mode_client(resp_client.action), resp_client.counter, resp_client.waiting_time);
    }
    free(arg);
    return NULL;
}


void send_clients_and_close(int nClients, char *mode){
    pthread_t threads[nClients];
    struct client_info *ci;


    pthread_mutex_init( &mutex_s, NULL);
    pthread_mutex_init( &mutex_c, NULL);
    pthread_cond_init( &cond_rd, NULL);
    pthread_cond_init( &cond_wr, NULL);
    
    
    if(sem_init(&sem_ratio,0,1) == -1)
        errx(EXIT_FAILURE, "sem init failed");

    for (int i = 0; i < nClients; i++){
        ci = malloc(sizeof(struct client_info));
        strcpy(ci->mode, mode);
        ci->tid = i;

        if(pthread_create(&threads[i], NULL, clientFunction, ci) != 0)
            errx(EXIT_FAILURE, "error creating client thread");
        
    }

    for (int i = 0; i < nClients; i++){ 
        if(pthread_join(threads[i], NULL) != 0)
            errx(EXIT_FAILURE, "error joining client thread");
    }
}
