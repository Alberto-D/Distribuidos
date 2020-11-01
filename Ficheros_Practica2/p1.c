#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "proxy.h"


int main () {

  set_name("P1");
  set_ip_port("0.0.0.0",8000);
  init_connection_client();

  // Esta función debe crear un thread the recepción de mensajes y volver
  // inmediatamente la ejecución a este proceso.
  init_recv_thread();

  // Notifica que estamos listos para apagarnos
  notify_ready_shutdown();

  // La siguiente linea no compila, debes establecer el tiempo correcto de Lamport.
  while (get_clock_lamport() < 5 ) { usleep(10000);}


  notify_shutdown_ack();
  printf("SHUTDOWN!\n");

  sleep(1);
  close_connection();
  return 0;
}
