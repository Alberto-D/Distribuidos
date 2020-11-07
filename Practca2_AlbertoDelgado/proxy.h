
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>

enum operations {
  READY_TO_SHUTDOWN = 0,
  SHUTDOWN_NOW,
  SHUTDOWN_ACK
};

struct message {
   char             origin[20];
   enum operations  action;
   unsigned int     clock_lamport;
};

// ** FUNCIONES COMUNES
// Establecer el nombre del proceso (para los logs y trazas), Ej: P1, P2, P3.
void set_name (char name[2]);

// Establecer ip y puerto
// Si se trata del cliente, se define ip y puerto donde conectar con el servidor.
// Si se trata del servidor, se define ip y puerto donde se pone a escuchar.
void set_ip_port (char* ip, unsigned int port);

// Obtiene el valor del reloj de lamport.
// Utilizalo cada vez que necesites consultar el tiempo.
int get_clock_lamport();

// cierra la conexión
// Asegurate de cerrar los sockets y descriptores de comunicacion.
int close_connection();

// ** FUNCIONES DE CLIENTE (P1, P3)
// Inicializa la conexión en modo cliente
int init_connection_client();

// Inicializa un thread que está constantemente escuchando en el socket.
void init_recv_thread ();

// Notificar que está lista para realizar el apagado (READY_TO_SHUTDOWN)
void notify_ready_shutdown();

// Notifica que va a realizar el shutdown correctamente (SHUTDOWN_ACK)
void notify_shutdown_ack();

// ** FUNCIONES DE SERVIDOR (P2)
// Inicializa la conexión en modo servidor
int init_connection_server();

// Espera por la notificación del cliente para apagarse
// Llamada bloqueante. Usala tantas veces como clientes necesites aceptar.
// El accept() debe implementarse en esta función.
int wait_shutdown_notication();

// Ejecuta el shutdown a través de RPC
// Esta función bloqueante debe enviar SHUTDOWN_NOW y esperar el SHUTDOWN_ACK del cliente.
// Parametro: char[2] que define el cliente a apagar, ej: "P1", "P3"
int shutdown_now(char client[2]);





