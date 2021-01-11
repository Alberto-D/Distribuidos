#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <pthread.h>
#include <sys/select.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include "proxy.h"
#include  <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>

# define NUMBER_OF_USERS  5
int threadno=0;

char *names[10] = {"jgines", "mfernandez", "rcalvo","abanderas","pcruz"};
int number_of_names=NUMBER_OF_USERS;
//Aray of structs use to syncronisaze.
struct sync_elements array_of_sync_clients[NUMBER_OF_USERS];


//Variables nedded for the socket connection
int sockfd =0;
int new_socket=0;
struct sockaddr_in address;
int addrlen = sizeof(address);

//Trhead for the client
pthread_t thread;
pthread_t threads[MAX_CLIENTS];
struct sockaddr_in clientaddr;
//the name to set in setname()
char name[MAX_USERNAME_SIZE];

// An state to see if the machine is waiting for a message, the last ack to check if it is the last message and a narray of sendchunks to use in the server
struct message recividos[MAX_CLIENTS];

//The list of registered users, its number and a list of struct client that saves various parameters of each client.


pthread_mutex_t lock;

int n =0;

char gip[MAX_SIZE];


//Common functions---------------------------------------------------------------------
void usage(void){
	fprintf(stderr, "./reader --username #name --chunks #number \n./writer --username #name --path #path  --chunks  0 1 2 ...\n");
	fprintf(stderr,"./server\n");
	exit(1);
}

void set_username (const char* username){
	strcpy(name, username);
	printf("Username is %s\n",name);
}

void set_ip_port (char* ip, unsigned int port) {
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    	perror("socket failed");
    	exit(EXIT_FAILURE);
  	}else{
		printf("Socket created \n");
	}

	bzero(&clientaddr, sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = inet_addr(ip);
	strcpy(gip,ip );
    clientaddr.sin_port = htons( port );
	
}

int open_file(char * strFileName){
	int fdin = open(strFileName, O_RDONLY);
	if(fdin < 0){
		err(1, "can't open input file");
	}
	return fdin;
}



//Funciones para clientes------------------------------

void check_arguments_reader(int argc, char const *argv[]){
	if(argc != 5){
		usage();
	}else if((strcmp(argv[1], "--username")||(strcmp(argv[3], "--chunks")))){
		usage();

	}
	return;
}

void check_arguments_writer(int argc, char const *argv[]){
	if((strcmp(argv[1], "--username")||(strcmp(argv[3], "--path"))||(strcmp(argv[5], "--chunks")) )){
		usage();
	}
	//Compruebo si los numeros estan ordenados de menor a mayor, 7e es el priemr numero en el array
	for(int i=7; i<argc;i++){
		if(atoi(argv[i])<=(atoi(argv[i-1]))){
			usage();
		}
	}
	return;
}


int init_connection_client() {
	if((connect(sockfd, (struct sockaddr*)&address, sizeof(address)))< 0){
		perror("conection failed");
	}else{
    	printf("Conection created\n");
	}
	return sockfd; 
}


int start_conection(struct message first){
	printf("\n\t Servidor escuchando en el puerto %u ip %s \n",clientaddr.sin_port,inet_ntoa(clientaddr.sin_addr));

	if(sendto(sockfd, &first, sizeof(first), 0,(struct sockaddr *)&clientaddr,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	struct message ack;
	if(recvfrom(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&clientaddr,&addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	printf("El puerto es %d y en action %d  \n",ack.port, ack.action );
	int puerto = ack.port;
	if((ack.action==CHUNKACK)&&(ack.port >0 )){
		printf("Confirmo el mensaje\n");
		
		return puerto;
	}
	return -1;
}

//Writer functions---------------------------------
int send_chunk(int id, char textc[],enum actions action){
	struct send_chunk chunk_to_send;

	chunk_to_send.action= action;

	strcpy(chunk_to_send.username, name);
	chunk_to_send.chunk_id=id;
	
	strncpy(chunk_to_send.data, textc, MAX_SIZE);

	if(sendto(sockfd, &chunk_to_send, sizeof(chunk_to_send), 0,(struct sockaddr *)&clientaddr,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	return wait_ack(clientaddr);
}

int send_sync(int id, char textc[],enum actions action){
	struct send_chunk chunk_to_send;
	chunk_to_send.action= action;
	strcpy(chunk_to_send.username, name);
	chunk_to_send.chunk_id=id;
	strncpy(chunk_to_send.data, textc, MAX_SIZE);

	if(sendto(sockfd, &chunk_to_send, sizeof(chunk_to_send), 0,(struct sockaddr *)&clientaddr,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	struct sync_file sync;
	if(recvfrom(sockfd, &sync, sizeof(sync), 0,(struct sockaddr *)&address,&addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	printf("Recivo el SYNC de %s, EL SUCCES ES %d action %d\n",sync.username, sync.success, sync.action );

	if(sync.action==SYNC)
	return SYNC;

	return -1;
}



//Reader functions---------------------------------------------------------------------
int ask_for_chunk(char textc[],int id){
		printf("\n\tIntento enviar al puerto %u ip %s \n",clientaddr.sin_port,inet_ntoa(clientaddr.sin_addr) );

	//Pregunta por el chunk id y en textc se copia el texto.
	struct get_chunk chunk_to_get;
	chunk_to_get.action = GETCHUNK;
	strcpy(chunk_to_get.username, name);
	chunk_to_get.chunk_id=id;
	//Envia un getchunk pregunatndo por elchunk con id id.
	printf("Mando un getchunk\n");
	if(sendto(sockfd, &chunk_to_get, sizeof(chunk_to_get), 0,(struct sockaddr *)&clientaddr,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	//Espera a recivir el chunk y copia el text
	struct send_chunk recived_chunk;
	if(recvfrom(sockfd, &recived_chunk, sizeof(recived_chunk), 0,(struct sockaddr *)&clientaddr,&addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	strncpy(textc,recived_chunk.data,MAX_SIZE);
	printf( "LLEga de %s con action %d \n ", recived_chunk.username, recived_chunk.action);
	//Si me llega algo que no es un send chunk fallo, igual copmprobar tambien el nombre
	if(recived_chunk.action != SENDCHUNK){
		return 1;
	}
	//Envia el asentimiento del chunk
	struct chunk_ack ack;
	ack.action= CHUNKACK;
	strcpy(ack.username, name);
	ack.chunk_id= id;
	if(sendto(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&clientaddr,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	
	return 0;
}

void close_connection(){
	close(sockfd);
}





//Server functions---------------------------------------------------------------------

int wait_ack(struct sockaddr_in address){
	struct chunk_ack asentimiento;
		if(recvfrom(sockfd, &asentimiento, sizeof(asentimiento), 0,(struct sockaddr *)&address,&addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}
	printf("Recivo el ack de %s, chunk_id %d action %d ",asentimiento.username, asentimiento.chunk_id, asentimiento.action );

	if( asentimiento.action== DECISION){
	printf( " pero no se escribira porque ya se ha realizado la sincronizacion \n");
	return DECISION;
	}
	if( asentimiento.action== CHUNKACK){
	printf( " \n");
	return CHUNKACK;
	}
	return -1;
}

//Int que hace que el programa continue.
int should_go_on;
int fd;	

//Variable booleana que voy a usar para comprobar si algun hilo está en fase de sincronizacion, si se pone a 1 significa que hay que sincronizar.
int syncronize = 0;
//Nombre del hilo que hay que sincronizar para que solo se sincronicen los que usan ese hilo.
char name_sync[MAX_USERNAME_SIZE];
//Variable para que los hilo subordinados esperen la respuesa del numero de conflictos del corordinador.
int wait_decision=-1;
int threads_read=0;
pthread_mutex_t lock_sync;
struct can_commit commit;

//Variable para ver el numero threads ejecutandose en un momento dado, cada vez que se acxaba un sync se pone a 0 porque los threads se cierran.
int number_of_threads =0;
//Numero de threads procesados en total antes de la ejecucion del ultimo sync, para saber cuantos llevo y poder restarlo en sync_activated.
//Cada vez que se procesa un syc_function se le suman los hilos que habia activos.
struct decision decisions[MAX_CLIENTS];





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


int init_socket (int* port_to_make,struct sockaddr_in clientaddr_to_set ) {
	int port = *(int*)port_to_make;

	int local_sd;
	if ((local_sd  = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    	perror("socket failed");
    	exit(EXIT_FAILURE);
  	}else{
		printf("Socket created \n");
	}

	bzero(&clientaddr_to_set, sizeof(clientaddr_to_set));
	clientaddr_to_set.sin_family = AF_INET;
    clientaddr_to_set.sin_port = htons( port );
	clientaddr_to_set.sin_addr.s_addr = inet_addr(gip);

	if (bind(local_sd, (struct sockaddr *)&clientaddr_to_set, sizeof(clientaddr_to_set))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }else{
	 	printf("Conection created\n");
	}
		
	return local_sd;
}



 /* Hilo trabajador lector  */
void* worker_thread_reader (void* r) {
	int recdata;
	int local_sd;
	int server_port;
	// Casting para leer datos del hilo
	struct thread_data rq = *( (struct thread_data*) r); 
	printf("\n Atendiendo a client [%s]  en %s : %d " , rq.username, inet_ntoa(rq.clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port));
	printf("\n Por el hilo servidor num %u \n", rq.thread_num);
	// Creamos un nuevo socket
	server_port = get_random(3000, 10000);
	struct sockaddr_in clientaddr; 
	socklen_t addrlen = sizeof(clientaddr);
	local_sd = init_socket(&server_port, clientaddr);
	// Se lo decimos al cliente
	struct message ack;
	ack.port= server_port;
	ack.action= CHUNKACK;
	sendto(rq.init_sd, &ack, sizeof(ack), 0, (struct sockaddr*) &rq.clientaddr, rq.addlen);

	char filename[MAX_USERNAME_SIZE+4];
	sprintf(filename,"%s.txt",rq.username);
	//Para que no sealicen lecturas ye scrituras a la vez se puede poner un lock a lo mejor? me parece extremo
	int fdin = open_file(filename);
	//Y en el while voy tramitando los mensajes, cada bucle es un mensaje tramitado
	struct get_chunk chunk_to_get;
	int go_on=1;
	//ACtivo el lock para que no haya lecturas y escrituras simultaneas
	pthread_mutex_lock(&lock_sync);
	while (go_on==1){
		// Limpiamos el bufer de datos		
		recdata = recvfrom (local_sd, &chunk_to_get, sizeof(chunk_to_get), 0, (struct sockaddr*)  &clientaddr,  &addrlen);
		printf("Recivido un getchunk \n");
		if (recdata <= 0)
		continue;
		struct send_chunk chunk_to_send;
		chunk_to_send.action= SENDCHUNK;
		chunk_to_send.chunk_id= chunk_to_get.chunk_id;
		chunk_to_send.data_size=MAX_SIZE;
		strcpy(chunk_to_send.username, chunk_to_get.username);
		memset(chunk_to_send.data, 0, MAX_SIZE);
		int read_status = read(fdin, chunk_to_send.data, MAX_SIZE);
		if(read_status < 0){
			fprintf(stderr, "can't read");
			exit(1);
		}
		//Si el chunk_id es -1 significa que es el ultimo mensaje, cierra conexion y el lock.
		if(chunk_to_get.chunk_id==-1){
			pthread_mutex_unlock(&lock_sync);
			go_on =0;
			chunk_to_send.data_size=0;
			memset(chunk_to_send.data, 0, MAX_SIZE);
		}//Mando el chunk-
		printf("\n He recibido: [%s](%d) desde %s : %d en el hilo %u\n" ,chunk_to_get.username , recdata, inet_ntoa (rq.clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port), rq.thread_num );         
		sendto (local_sd, &chunk_to_send, sizeof(chunk_to_send), 0, (struct sockaddr*) &clientaddr,  addrlen);
		//Espero el ack.
		struct chunk_ack ack;
		recvfrom (local_sd, &ack, sizeof(ack), 0, (struct sockaddr*)  &clientaddr,  &addrlen);

	}
	printf("Cerramos hilo (%d) ...\n", rq.thread_num);
	close (local_sd);
	return NULL;
}

//Utilizo esta funcion para saber cuantos bytes hay que copiar, ya que por algun motivo si pongo strlen(strs) en write se suma el nombre del archivo al texto.
int get_size(char strs[] ){
	int size = strlen(strs);
	if (size>=MAX_SIZE){
		size = MAX_SIZE;
	}	
	return size;
}

//Limpia el array poniendo los parametros de todos los elementos a -1, numero que no puedentener de forma natural.
void clean_decisions(struct decision decisions[]){
	for(int i =0; i< MAX_CLIENTS;i++){
		decisions[i].action=-1;
		decisions[i].isOk=-1;
		decisions[i].thread_id=-1;
	}
}

// Consigo la id del usuario e "inicializo" su syncronize a 0.
int get_client_id(char client_name[]){
	for (int i = 0; i<NUMBER_OF_USERS;  i++){
		if(strncmp(names[i], client_name,strlen(client_name))==0){
			pthread_mutex_lock(&lock_sync);
			array_of_sync_clients[i].syncronize=0;
			pthread_mutex_unlock(&lock_sync);
			return i;
		}
	}
	return -1;
}

int sync_function(int thread_number, int list_to_copy[],int number_of_chunks, int user_id){

	//Si solo hay un thread de ese cliente no hace falta sincronizar nada, así que vuelvo y pongo el numero a 0 porque ese hilo ya ha acabado.
	// Al haber solo un hilo del cliente, no hace falta usar lock.
	if(array_of_sync_clients[user_id].number_of_threads==1){
		array_of_sync_clients[user_id].number_of_threads=0;
		return 0;
	}
	printf("		El hilo %d entra en sync_function y es maestro\n",thread_number);

	//Cojo el lock y cambio syncronize a 1, todos los threads de este cliente tienen sync a 1 ahora por lo que entraran en sync activated.
	//Tambien pongo el can commit con los datos necesarios y suelto el lock, ahora el resto de threads pueden leer
	pthread_mutex_lock(&lock_sync);
		array_of_sync_clients[user_id].syncronize=1;

		array_of_sync_clients[user_id].wait_decision=-1;
		array_of_sync_clients[user_id].subordinated_threads=0;
		array_of_sync_clients[user_id].threads_read=0;

		array_of_sync_clients[user_id].commit.action=CANCOMMIT;
		for(int i=0; i<number_of_chunks; i++){
        	array_of_sync_clients[user_id].commit.chunk_list[i] = list_to_copy[i];
		}
		array_of_sync_clients[user_id].commit.num_chunks= number_of_chunks;
	pthread_mutex_unlock(&lock_sync);

	//Espero a que todos los threads hayan leido y contestado al mensaje escribiendo en el array.
	while(array_of_sync_clients[user_id].threads_read != (array_of_sync_clients[user_id].number_of_threads-1)){
		usleep(100);
	}
	//Una vez todos los treads han escrito en en array decisions leo las respuestas, si hay algun conflicto error suma.
	int error = 0;
	for (int i =0; i< array_of_sync_clients[user_id].subordinated_threads ;i++){
		if(array_of_sync_clients[user_id].decisions[i].isOk==0){
			error++;
		}
	}
	//Una vez he comprobado todos los conflictos pongo wait_decision a 1, el resto de hilos ya pueden seguir ejecutando.
	pthread_mutex_lock(&lock_sync);
		array_of_sync_clients[user_id].wait_decision=error;
	pthread_mutex_unlock(&lock_sync);

	//Antes de acabar pongo las variables usada a 0 y limpio el array de desions para poer volver a usarlo.
	array_of_sync_clients[user_id].number_of_threads =0;
	clean_decisions(array_of_sync_clients[user_id].decisions);
	return error;
}

int sync_activated(int all_thread_number, int list_to_check[],int number_of_chunks, int user_id){
	//Para que cada thread escriba en un espacio del array de decisiones uso el numero de threads procesados, que aumenta cada vez que un hulo entra en la funcion
	pthread_mutex_lock(&lock_sync);
	int thread_number = array_of_sync_clients[user_id].subordinated_threads;
	array_of_sync_clients[user_id].subordinated_threads++;
	pthread_mutex_unlock(&lock_sync);

	printf("		El hilo subordinado %d de %s entra en sync_activated \n ",thread_number,names[user_id] );
	array_of_sync_clients[user_id].decisions[thread_number].action = DECISION;
	array_of_sync_clients[user_id].decisions[thread_number].thread_id= thread_number;
	//Si algun chunk de mi lista es igual alguno de la de commit del hilo que ha empezado el sync, isok=0;, si no es 1.
	int colitions=1;
	for(int i =0;i<number_of_chunks;i++){
		for(int j =0; j<array_of_sync_clients[user_id].commit.num_chunks;j++){
			if(list_to_check[i]==array_of_sync_clients[user_id].commit.chunk_list[j]){
				printf("Server - Conflicto detectado en el chunk %d \n",i);
				colitions=0;
			}
		}
	}
	array_of_sync_clients[user_id].decisions[thread_number].isOk=colitions;
	//En el array de decisiones, pongo la que acabo de crear, que indica si hay problemas o no.
	//Sumo al contador usado el lock para indicar que este hilo ya ha colocado su decision.
	pthread_mutex_lock(&lock_sync);
		array_of_sync_clients[user_id].threads_read++;
	pthread_mutex_unlock(&lock_sync);

	//Espero a que el hilo coordinador acaba de ver los mensajes decision para saber si hay más de dos conflicos.
	while(array_of_sync_clients[user_id].wait_decision<0){
		usleep(100);
	}
	//Si es o o 1 significa que  no hay conflicto o hay un solo conflicto, luego es coordinador desecha su texto pero al devolver 0 los subordinados escriben.
	if(array_of_sync_clients[user_id].wait_decision<=1)
		return 0;
	//Si wait_decision no es 1 significa que hay más de 1 conflicto, luego no se escribe nada.
	return 1;
}


void write_final(int sync,char filename[],  char username[], int chunk_list[], int number_of_chunks){
	char final_filename[MAX_USERNAME_SIZE+4];
	sprintf(final_filename,"%s.txt",username);

	//Abro el archivo temporal para leer.
	char buffer[MAX_SIZE];
	int fptwrite;

	//Si sync ==0 no ha habido ningun conflicto, abro los archivos y escribo.
	//Si ha habido conflicto, sync será distinta de 0 con lo cual no pasa nada
	if(sync==0){
		pthread_mutex_lock(&lock_sync);
//Pongo el lock para que los archivos no puedan estar abierto por varios threads a la vez
		int fptread= open_file(filename);
		if((fptwrite = open(final_filename, O_WRONLY | O_CREAT , 0777))<0){
			perror("Failed open");
			exit(EXIT_FAILURE);            
		}
		//Voy comprobando todo el array de los chunks que hay que escribir, como an de menor a mayor vale con usar un for.
		int chunk_index =0;
		for(int i =0;i<=chunk_list[number_of_chunks-1];i++){
			//Si es un chunk de la lista lo escribo y muevo el index de la lista.
			if(i == chunk_list[chunk_index]){
				if(read(fptread, buffer, MAX_SIZE)<0){
					printf("Read failled\n");
					exit(EXIT_FAILURE);	
				}
				if(write(fptwrite,buffer, get_size(buffer))<0){
					printf("Writing failled\n");
					exit(EXIT_FAILURE);	
				}
				chunk_index++;
			}else{
				//Si no, muevo el cursor al siguiente chunk.
				int eso;
				if((eso = lseek(fptwrite, MAX_SIZE, SEEK_CUR))<0){
					perror("Failed lseek");
					exit(EXIT_FAILURE); 
				}
			}	
		}
//Una vez acabada la escritura cierro los archivos y libero el lcok.
		close(fptread);
		close(fptwrite);
		pthread_mutex_unlock(&lock_sync);
	}
	//Para acabar indico que ha abacado la escritura y borrro el fuchero temporal.
	printf("Escritura finalizada\n");
	remove(filename);

	return;
}

 /* Hilo trabajador escritor  */
void* worker_thread_writer (void* r) {
	int recdata;
	int local_sd;
	int server_port;

	// Casting para leer datos del hilo
	struct thread_data rq = *( (struct thread_data*) r); 
	printf("\n Atendiendo a client [%s]  en %s : %d " , rq.username, inet_ntoa(rq.clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port));
	printf("\n Por el hilo servidor num %u \n", rq.thread_num);
	// Creamos un nuevo socket
	server_port = get_random(3000, 10000);
	// Saco la id del usuario para poder sincronizar más adelante.
	int user_id = get_client_id(rq.username);

	struct sockaddr_in clientaddr; 
	socklen_t addrlen = sizeof(clientaddr);
	local_sd = init_socket(&server_port, clientaddr);
	printf("El numero es %d \n", server_port);
	// Se lo decimos al cliente
	struct message ack;
	ack.port= server_port;
	ack.action= CHUNKACK;
	sendto(rq.init_sd, &ack, sizeof(ack), 0, (struct sockaddr*) &rq.clientaddr, rq.addlen);
	//A partir de este punto comienza la comunicacion de verdad, abro el archivo temporal y empiezo.
	int  fptr;
	char filename[MAX_USERNAME_SIZE+10];
	sprintf(filename,"%s-%d.txt",rq.username,ntohs (rq.clientaddr.sin_port));
	//Abro el archivo temporal, si existe  lo sobre escribo
	if((fptr = open(filename, O_WRONLY | O_CREAT, 0777))<0){
		perror("Failed open");
		exit(EXIT_FAILURE);            
	}
  	int chunk_list[MAX_SIZE]; 
   	memset(chunk_list,0,MAX_SIZE);
	// Lazo principal
	struct send_chunk recived;
	int go_on=1;
	int index =0;
	while (go_on==1){
		//Limpio el buffer y recivo el mensaje para escribir.
		memset(recived.data,0,MAX_SIZE);	
		if(recvfrom (local_sd, &recived, sizeof(recived), 0, (struct sockaddr*)  &clientaddr,  &addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		} 	
		//Asiento el mensaje y lo escribo en el archivo temporal.
		struct chunk_ack asentimiento;
		asentimiento.action=CHUNKACK;
		asentimiento.chunk_id=recived.chunk_id;
		strcpy(asentimiento.username,recived.username);
		if(recived.action==SENDCHUNK){	
			chunk_list[index]= recived.chunk_id;
			index++;
			if(write(fptr,recived.data, get_size(recived.data))<0){
				printf("Writing failled\n");
				exit(EXIT_FAILURE);	
			}
			usleep(1000);
			//Compruebo si hay que sincronizar.
			if((array_of_sync_clients[user_id].syncronize)){
				int sync = sync_activated(rq.thread_num, chunk_list, index,user_id);
				go_on=0;
				asentimiento.action=DECISION;
				close(fptr);
				write_final(sync, filename, rq.username, chunk_list,index);
			}
		}else if(recived.action==SYNC){
			//Si es el mensaje sync proceso la sincronizacion, emvio el mensaje y salgo cerrando el archivo, escribiendo en el archivo final y cerrando el socket.
			go_on=0;
			int sync = sync_function(rq.thread_num, chunk_list, index,user_id);
			struct sync_file sync_message;
			strcpy(sync_message.username, asentimiento.username);
			sync_message.success = sync;
			sync_message.action = SYNC;
			if(sendto (local_sd, &sync_message, sizeof(sync_message), 0, (struct sockaddr*) &clientaddr,  addrlen)<0){
				perror("Failed sendto");
				exit(EXIT_FAILURE);
			}
			close(fptr);

			write_final(sync, filename, rq.username, chunk_list,index);
			printf("Cerramos hilo (%d) ...\n", rq.thread_num);
			close (local_sd);	
			return NULL;
		}
		
		if(sendto (local_sd, &asentimiento, sizeof(asentimiento), 0, (struct sockaddr*) &clientaddr,  addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}
		
		printf("\n He recibido: [%s](%d) desde %s : %d en el hilo %u\n" ,asentimiento.username , recdata, inet_ntoa (rq.clientaddr.sin_addr), ntohs (rq.clientaddr.sin_port), rq.thread_num );         
	}
	printf("Cerramos hilo (%d) ...\n", rq.thread_num);
	array_of_sync_clients[user_id].syncronize = 0;

	close (local_sd);
	return NULL;
}




int wait_client(char *names[], int number_of_names){
	int recdata;
	// direccion del cliente
	struct sockaddr_in clientaddr; 
	socklen_t addrlen = sizeof(clientaddr);
	// Buffer de datos UDP, lo uso para ver si el cliente esta registrado y sies lector o escritor
	struct message first_message;

	int port;
	if(pthread_mutex_init(&lock_sync, NULL) != 0){
        printf("\n mutex init failed\n");
        exit(1);
    }
	

	// TODO: pasar como parametros
	port = 8128;
	fd = init_socket(&port, clientaddr);

	// gestion de interrupciones
	signal(SIGINT, sig_handler);
	signal(SIGTSTP, sig_handler);
	// unbuffer stdout: para ver las salidas de los hilos 
	setvbuf(stdout, NULL, _IONBF, 0); 
	should_go_on = 1;
	printf("\n\t Servidor escuchando en el puerto %u \n",port );
		
	// Lazo principal de ejecucion
	while (should_go_on==1){   
		// limpiamos buffer de datos, pniendo a -1 la action ya que es un estao en el que no puede estar.
		memset (first_message.username, 0, MAX_USERNAME_SIZE);
		first_message.action=-1;
		// Aqui esperamos nuevos clientes

		recdata = recvfrom (fd, &first_message, sizeof(first_message), 0, (struct sockaddr*) &clientaddr, &addrlen);
		if (recdata <= 0 )
		continue;

		if (is_registred(first_message.username, names, number_of_names)){
			//Si esta registrado, una vez recibida una peticion, preparamos los datos del hilo que lo atendera.
			struct thread_data *r = malloc( sizeof( struct thread_data ) );  
			bzero (r, sizeof (struct thread_data));  
			r->thread_num = threadno;
			r->addlen = addrlen;
			r->clientaddr = clientaddr;
			r->init_sd = fd;
			strcpy (r->username, first_message.username);
			// Y creamos el hilo para atenderlo, que trabajara en un hilo distinti dependiendo de si es lector o escritor.
			if(first_message.action==READER){
				pthread_create (&threads [threadno++], NULL, worker_thread_reader, (void*)r);
			}else if(first_message.action==WRITER){
				pthread_create (&threads [threadno++], NULL, worker_thread_writer, (void*)r);

			}
			array_of_sync_clients[get_client_id(first_message.username)].number_of_threads++;
			if (threadno == MAX_THREADS){
				threadno = 0;
				printf("\n Demasiados hilos creados? ... \n");
			}
				
		//Si no está registrado le envio un mensaje con el puerto a -1 que hace que el cliente cierre con error y el while sigue ejecutandose.
		}else{
			fprintf(stderr,"User %s is not registered",first_message.username);
			struct message ack;
			ack.port= -1;
			ack.action= CHUNKACK;
			sendto(fd, &ack, sizeof(ack), 0, (struct sockaddr*) &clientaddr, addrlen);
		}
				
	}
	return 0;
}

int is_registred(char username[],char *strs[], int size){
	int exit=0;
	for(int i =0;i< size ;i++){
		if(strncmp(strs[i], username,strlen(username))==0){
			exit=1;
		}
	}
	return exit;
}