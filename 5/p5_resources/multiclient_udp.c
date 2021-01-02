/***********************************************************************************************
 *                                                                                             *
 *  Servidor UDP Multithread.                                                                  *
 *  Basado en https://github.com/AnkitDimri/multithread-udp-server.git                         *
 **********************************************************************************************/

#include "multiclient_udp.h"

/* Gestion de interrupciones: cerramos socket antes de salir */
void sig_handler(int signo) {
  if (signo == SIGINT) {
    printf("\nTerminando programa...\n");
    close (fd);
    should_go_on = 0;
    for (int i = 0; i < threadno; i++) {
      pthread_join(threads[i], NULL);
    }
    printf("\nTodos los thread han terminado.\n");            
  }
}

/* Genera un entero aleatorio en el rango indicado */
int get_random(int lower, int upper) { 
  int num = (rand() % (upper - lower + 1)) + lower; 
  return num;
} 

 /* Hilo trabajador  */
void* worker_thread (void* r) {
  int recdata;
  int local_sd;
  int server_port;
  char local_buf [MAX_SIZE]; // Buffer de datos UDP
  // Casting para leer datos del hilo
  struct thread_data rq = *( (struct thread_data*) r ); 
  printf("\n Atendiendo a client [%s]  en %s : %d " , rq.username, inet_ntoa(rq.clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port));
  printf("\n Por el hilo servidor num %u \n", rq.thread_num);
  // Creamos un nuevo socket
  server_port = getRandom(3000, 10000);
  local_sd = init_socket(&server_port);
  // Se lo decimos al cliente
  sprintf(local_buf, "%u", server_port);
  sendto(rq.init_sd, local_buf, strlen (local_buf), 0, (struct sockaddr*) &rq.clientaddr, rq.addlen);
  // Lazo principal
  while (should_go_on==1) {
    // Limpiamos el bufer de datos
    memset (local_buf, 0, sizeof (local_buf)); 
    recdata = recvfrom (local_sd, local_buf, 2048, 0, (struct sockaddr*)  &rq.clientaddr, &rq.addlen);
    if (recdata <= 0 )
      continue;
    printf("\n He recibido: [%s](%d) desde %s : %d en el hilo %u" , local_buf, recdata, inet_ntoa (rq.clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port), rq.thread_num );         
    strrev(local_buf);
    sendto (local_sd, local_buf, strlen (local_buf), 0, (struct sockaddr*) &rq.clientaddr, rq.addlen);
  }
  printf("Cerramos hilo (%d) ...\n", rq.thread_num);
  close (local_sd);
  return NULL;
}

int main(int argc, char const *argv[]) {
  int recdata;
  // direccion del cliente
  struct sockaddr_in clientaddr; 
  socklen_t addrlen = sizeof(clientaddr);
  // Buffer de datos UDP
  char buf [MAX_SIZE]; 
  int port;
  // TODO: pasar como parametros
  port = 8128;
  fd = init_socket(&port);

  // gestion de interrupciones
  signal(SIGINT, sig_handler);
  signal(SIGTSTP, sig_handler);
  // unbuffer stdout: para ver las salidas de los hilos 
  setvbuf(stdout, NULL, _IONBF, 0); 
  should_go_on = 1;
  printf("\n\t Servidor escuchando en el puerto %u \n",port);
     
  // Lazo principal de ejecucion
  while (should_go_on==1) {   
    // limpiamos buffer de datos
    memset (buf, 0, sizeof (buf));       
    // Aqui esperamos nuevos clientes
    recdata = recvfrom (fd, buf, 2048, 0, (struct sockaddr*) &clientaddr, &addrlen);
    if (recdata <= 0 )
      continue;
    // una vez recibida una peticion, preparamos los datos del hilo que lo atendera
    struct thread_data *r = malloc( sizeof( struct thread_data ) );  
    bzero (r, sizeof (struct thread_data));  
    r->thread_num = threadno;
    r->addlen = addrlen;
    r->clientaddr = clientaddr;
    r->init_sd = fd;
    strcpy (r->username, buf);
    // Y creamos el hilo para atenderlo, que trabajara en otro hilo
    pthread_create (&threads [threadno++], NULL, worker_thread, (void*)r);
    if (threadno == MAX_THREADS){
      threadno = 0;
      printf("\n Demasiados hilos creados? ... \n");
    }
            
  }
  return 0;
}

