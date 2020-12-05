
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#define MAX_SIZE 512

struct send_chunk{
   char username[MAX_SIZE];
   int chunk_id;
   int data_size;
   char data[MAX_SIZE];
};

struct chunk_ack{
   char username[MAX_SIZE];
   int chunk_id;
};

void usage(void);

void check_arguments_client(int argc, char const *argv[]);

void *thread_reception(void *unused);


int is_registred(char username[],char *strs[], int size);
// ** FUNCIONES COMUNES
// Establecer el username del client (para los logs y trazas).
void set_username (char* username);

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
int send_ack(int sockfd, int chunk_id, char* username);