#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "proxy.h"


int main () {

  set_name("P3");
  set_ip_port("127.0.0.1",8000);
    //Para usar maquinas distintas hay que poner la direccion inet del server 

  init_connection_client();

  // Esta función debe crear un thread the recepción de mensajes y volver
  // inmediatamente la ejecución a este proceso.
  init_recv_thread();

  // Notifica que estamos listos para apagarnos
  notify_ready_shutdown();

  // La siguiente linea no compila, debes establecer el tiempo correcto de Lamport.
  while (get_clock_lamport() < 9 ) { usleep(10000);}


  notify_shutdown_ack();
  printf("SHUTDOWN!\n");

  sleep(1);
  close_connection();
  return 0;
}
