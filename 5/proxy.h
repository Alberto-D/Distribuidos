
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>

#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define MAX_THREADS 128
#define MAX_USERNAME_SIZE 50
#define MAX_SIZE 512

#define PORT 8128
#define MAX_CLIENTS 128





pthread_t threads [MAX_THREADS];
int fd;
int should_go_on;


enum actions {
   GETCHUNK = 0,
   SYNC = 1,
   CANCOMMIT = 2,
   DECISION = 3,
   SENDCHUNK = 4,
   CHUNKACK = 5
};
enum clients {
   READER = 0,
   WRITER = 1,
   CONFIRM = 2,
   DENNY=3

};

//Lectura

struct get_chunk{
   enum actions action;
   char username[MAX_USERNAME_SIZE];  
   int chunk_id;
};

//Excrituras concurrentes
struct sync_file{
   enum actions action;
   char username[MAX_USERNAME_SIZE];
   int success;
};
struct can_commit{
   enum actions action;   
   int chunk_list[512];  
   int num_chunks;
};
struct decision{  
   enum actions action;
   int thread_id;
   int isOk;
};

//Comunes


struct send_chunk{
   enum actions action;
   char username[MAX_USERNAME_SIZE];
   int chunk_id;
   int data_size;
   char data[MAX_SIZE];
};

struct chunk_ack{   
   enum actions action;
   char username[MAX_USERNAME_SIZE];
   int chunk_id;
};

   



//////




struct thread_data {
    int thread_num;
    int init_sd;
    char username[MAX_USERNAME_SIZE];
    socklen_t addlen;
    struct sockaddr_in clientaddr;
};

struct message{
   enum actions action;
   char username[MAX_USERNAME_SIZE];
   int port;
};


struct sync_elements{
   int syncronize ;
   int wait_decision ;
   int number_of_threads;
   int threads_read;
   int subordinated_threads;
   struct can_commit commit;
   struct decision decisions[MAX_CLIENTS];
};


// Indica la forma decuda de usar el programa y sale.
void usage(void);

int get_client_id(char client_name[]);
// Comprueban los arumentos del lector y el escritor.
void check_arguments_reader(int argc, char const *argv[]);
void check_arguments_writer(int argc, char const *argv[]);

// Establecer el username del client (para los logs y trazas).
void set_username (const char username[]);

// Pone la ip y el puerto indicados por parametros.
void set_ip_port (char* ip, unsigned int port); 

// Comienza la conexion del cliente
int init_connection_client();

// Toma el nombre de un archivo y devuelve un ID de archivo o -1 si falla.
int open_file(char * strFileName);

// Manda un primer mensaje con si es escritor o lector para empezar la comunicacacion y espera un ack.
// Si llega un CHUNKACK la conexcion está bien y se puede seguir, si no, devuelvo error.
int start_conection(struct message first);

// Manda por la direccion del cliente (designada en init_conection_client) un chunk con las caracteristicas de los parametros.
int send_chunk(int id, char textc[],enum actions action);

// Envia un mensaje de sincronizacion al servidor para que sepa que ya ha terminadode enviar chunks y puede escribir en el fichero final.
int send_sync(int id, char textc[],enum actions action);



//Envia un mensaje preguntando por el chunk id al servidor, en texctc se guarda el texto de ese chunk.
int ask_for_chunk(char textc[],int id);

//Cierra la conexion.
void close_connection();


//Espera un mensaje ack en la direccion address.
int wait_ack(struct sockaddr_in address);


// Gestion de interrupcions: cerramos sockets y esperamos hilos antes de salir 
void sig_handler(int signo);

// Genera un entero aleatorio en el rango indicado 
int get_random(int lower, int upper);

// Devuelve un socket con el puerto y la direccion indicados.
int init_socket (int* port_to_make,struct sockaddr_in clientaddr_to_set );


// Hilo que controla el funcionamiento del server con un cliente lector.
void* worker_thread_reader (void* r);


// Limpia el array poniendo los parametros de todos los elementos a -1, numero que no puedentener de forma natural.
void clean_decisions(struct decision decisions[]);

// Funcion que se activa cuando un hilo tiene que sincronizar.
// Lo combierte en el hilo maestro y maneja la sincronizacion de todos los hilos.
int sync_function(int thread_number, int list_to_check[],int number_of_chunks,int user_id);

// Funcion que se ejecuta en los hilos subordinados cuando e activa la sincronizacion.
int sync_activated(int all_thread_number, int list_to_check[],int number_of_chunks, int user_id);

// Escribe los datos en el archivo final y borra el temporal.
// Dependiendo del parametro sync se escribe o no.
void write_final(int sync,char filename[], char username[], int chunk_list[], int chunk_number);

// Hilo que controla el funcionamiento del server con un cliente escritor.
void* worker_thread_writer (void* r);

// Funcion principal queespera la llegada de mnsajes y los pasa a los hilos.
int wait_client(char *names[], int number_of_names);

// Funcion que comprueba si el usuario está registrado.
int is_registred(char username[],char *strs[], int size);
