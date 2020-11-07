#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "proxy.h"

int main () {
  int clients[2];

  set_name("P2");
  set_ip_port("127.0.0.1",8000);
    //Para usar maquinas distintas hay que poner la direccion inet del server 

  init_connection_server();

  // Llamada bloqueante que espera por nuevas conexiones (2 clientes)
  wait_shutdown_notication(); 
  wait_shutdown_notication();

  //
  // Escribe aquí tu código
  //
  shutdown_now("P1");
  shutdown_now("P3");


  printf("Clientes fueron correctamente apagados en t(lamport) = %d\n", get_clock_lamport());

  sleep(1);
  close_connection();

  return 0;
}
