
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#define MAX_SIZE 512
#define MAX_USERNAME_SIZE 512


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
struct message{
   enum actions action;
   char username[MAX_USERNAME_SIZE];
};




void check_arguments_reader(int argc, char const *argv[]);
void check_arguments_writer(int argc, char const *argv[]);


int start_conection(struct message first);


void *thread_server_writer(void *oldi);
void *thread_reception_reader(void *unused);

int send_chunk(int id, char textc[],enum actions action);
int wait_ack(struct sockaddr_in address);
int send_ack(struct sockaddr_in address,int chunk_id,char name[]);


void usage(void);


int ask_for_chunk(char recived[], int chunck_id);


int is_registred(char username[],char *strs[], int size);
// ** FUNCIONES COMUNES
// Establecer el username del client (para los logs y trazas).
void set_username (const char username[]);

// Establecer ip y puerto
// Si se trata del cliente, se define ip y puerto donde conectar con el servidor.
// Si se trata del servidor, se define ip y puerto donde se pone a escuchar.
   void set_ip_port (char* ip, unsigned int port); 
// Toma el nombre de un archivo y devuelve un ID de archivo o -1 si falla.
   int open_file(char * strFileName);
// Cierra el fichero cuando termina de enviarlo o escribir en él
   int close_file(int fd);
// cierra la conexión
// Asegurate de cerrar los sockets y descriptores de comunicacion.
   int close_connection();

// ** FUNCIONES DE CLIENTE
// Inicializa la conexión en modo cliente
    int init_connection_client();
// Inicializa un thread que está constantemente escuchando en el socket.
   void init_recv_thread (void);
// Escribe datos en el socket y devuelve bytes escritos.
   int write_block(int fd, char * strData, int byteOffset, int blockSize);

// ** FUNCIONES DE SERVIDOR (P2)
// Inicializa la conexión en modo servidor
   int init_connection_server();
// Espera a un nuevo cliente
   int wait_client(char *names[], int number_of_names);
// Confirma la recepción de un chunk
