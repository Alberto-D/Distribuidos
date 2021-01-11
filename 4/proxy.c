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


#define PORT 8128
#define MAXI 1024
#define MAX_CLIENTS 100
#define MAX_CHAR 20
#define MAX_SIZE 512
#define Recived 1
#define Waiting 2

//Variables nedded for the socket connection
int sockfd =0;
int new_socket=0;
struct sockaddr_in address;
//Trhead for the client
pthread_t thread;
//Two arrays of 100, one of threads and the ohter of adresses, so that the server sends each packet to the correct client
struct sockaddr_in addresses[MAX_CLIENTS];
pthread_t threads[MAX_CLIENTS];

//the name to set in setname()
char name[MAXI];

// An state to see if the machine is waiting fro a message, the last ack to check if it is the last message and a narray of sendchunks to use in the server
int estado=Recived;
int last_ack=0;
struct send_chunk recividos[MAX_CLIENTS];

//The list of registered users, its number and a list of struct client that saves various parameters of each client.
char *names[10] = {"jgines", "mfernandez", "rcalvo","abanderas","pcruz"};
int number_of_names=5;
pthread_mutex_t lock;
struct client list[10];

int n =0;
//The current date goes here, so that two files never have the same name.
char date[MAXI];



//Common functions---------------------------------------------------------------------
void usage(void){
	fprintf(stderr, "./client --username #name --path #path\nServer : ./server --priority writer/reader\n");
	exit(1);
}

void set_username (char* username){
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

	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons( port );
}

int open_file(char * strFileName){
	
	int fdin = open(strFileName, O_RDONLY);
	if(fdin < 0){
		err(1, "can't open input file");
	}
	return fdin;
}

int close_file(int fd){
	close(fd);
}
int close_connection(){
	pthread_join(thread,NULL);
	//close(sockfd);

}



//Client functions---------------------------------------------------------------------
void check_arguments_client(int argc, char const *argv[]){
	if(argc != 5){
		usage();
	}else if((strcmp(argv[1], "--username")||(strcmp(argv[3], "--path")))){
		usage();

	}else if( access( argv[4], F_OK ) == -1 ){
		usage();
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

void init_recv_thread () {
//Iniciate the listening thread for the client
  pthread_create(&thread, NULL, thread_reception, NULL);
}

void *thread_reception(void *unused){
	struct chunk_ack chunk;
	int addrlen = sizeof(address);
	//Allways reciving, if something is recived, state changes to Recived 
	while(1){
		int eso =recvfrom(sockfd, &chunk, sizeof(chunk), 0,(struct sockaddr *)&address,&addrlen);
		if(eso>0){
			estado=Recived;
			printf("Ack %d recived form %s\n",chunk.chunk_id,chunk.username );
			if(chunk.chunk_id-last_ack <MAX_SIZE){
				//If this ack-last ack is <512, it is the last message
				printf("LAst ack recived\n " );
				break;
			}
		}
		last_ack=chunk.chunk_id;
	}
  	return 0;
}

int write_block(int fdin, char * strData, int byteOffset, int blockSize){
	struct timeval t1, t2;
	long double time_taken = 0.0;
	if (gettimeofday(&t1, NULL)!= 0){
		perror("Error in get time of the day");
	}
	int addrlen = sizeof(address);
	struct send_chunk tosend;
	while(time_taken < 5){
		if(estado==Recived){
			//If state is recived it means that i have recived a message, so i send one, cheking if it fails, and state is now waiting because the client is waiting for an ack
			
			

			memset(tosend.data, 0, MAX_SIZE);
			strcpy(tosend.username, name);
			tosend.chunk_id=byteOffset;
			tosend.data_size=blockSize;
			strncpy(tosend.data, strData,blockSize);
			if(sendto(sockfd, &tosend, sizeof(tosend), 0,(struct sockaddr *)&address,addrlen)<0){
				perror("Failed sendto");
				exit(EXIT_FAILURE);
			}
			estado=Waiting;
			return 0;
		}else if (estado==Waiting){
			//If state is waiting, it sleeps 0.5 seconds
			usleep(500);
		}
		//I mesaure the time to check if it is 5 seconds
		if (gettimeofday(&t2, NULL)!= 0){
			perror("Error in get time of the day");
		}
		time_taken = (t2.tv_sec - t1.tv_sec)*1000;
		time_taken += (t2.tv_usec - t1.tv_usec) / 1000.0;
		time_taken = time_taken/1000;		
	}
	//If 5 seconds have already passed, y re-send it 
	printf("\nWaiting time execed, sending again\n");
	if(sendto(sockfd, &tosend, sizeof(tosend), 0,(struct sockaddr *)&address,addrlen)<0){
		perror("Failed sendto");
		exit(EXIT_FAILURE);
	}
	
}


//Server functions---------------------------------------------------------------------

int init_connection_server() {
   if (bind(sockfd, (struct sockaddr *)&address, sizeof(address))<0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }else{
	 	printf("Conection created\n");
	}
  return 0; 
}
//Checks if a client already exists in the list, if it doesnt tit is added
int get_client(char name[]){
	for(int i =0; i<10;i++){
		if((strncmp(list[i].username, name,strlen(name))==0)){
			list[i].is_first=0;
			return i;
		}else if(list[i].is_first){
			struct client new;
			strcpy(new.username, name);
			new.user_id=i;
			new.number++;
			list[i]= new;
			return i;
		}
	}
}

void *thread_server(void *oldi){	 
	FILE *fptr;
  	int addrlen = sizeof(address);
	struct chunk_ack ack;

	int i = *(int*)oldi;
	struct send_chunk recivido = recividos[i];

	if(is_registred(recivido.username, names, number_of_names)){
		//If the user is registered, i make a folder with its name, if it already exists i do nothing
		if((mkdir(recivido.username, 0700)<0)&&(errno !=EEXIST)){
			perror("mkdir failed");
			exit(EXIT_FAILURE);	
		}
		//I configure and send the ack, if it wails i exit
		strcpy(ack.username, recivido.username);
		ack.chunk_id = recivido.chunk_id;
		
		if(sendto(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&addresses[i],addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}		
		//I take the lock, make the nameof the file and write on it
		pthread_mutex_lock(&lock);
		int eso = get_client(recivido.username);
		char filename[MAXI];
		sprintf(filename,"%s/%s.%s-%d.txt", list[eso].username,list[eso].username,date,list[eso].number);
		printf(" Coping ack %d in  %s from %s\n",ack.chunk_id, filename,list[eso].username);

		if((fptr = fopen(filename,"a")) == NULL){
			printf("Error!");   
			exit(1);             
		}
		if(fwrite(recivido.data, strlen(recivido.data), 1 , fptr)<0){
			printf("Writing failled\n");
			exit(EXIT_FAILURE);	
		}
		fclose(fptr);
		if((recivido.data_size<MAX_SIZE)&&(!list[eso].is_first)){		
			printf("Finished coping %s\n",filename);
			list[eso].number++;
		}
		pthread_mutex_unlock(&lock);
	}else{
		//If the user is not registered, i send an ack whit id -1.
		fprintf(stderr,"%s :User unknown \n",recivido.username );
		strcpy(ack.username, recivido.username);
		ack.chunk_id = -1;
		if(sendto(sockfd, &ack, sizeof(ack), 0,(struct sockaddr *)&addresses[i],addrlen)<0){
			perror("Failed sendto");
			exit(EXIT_FAILURE);
		}
 	  	return 0;
  	}
  	return 0;
}

int wait_client(char *names[], int number_of_names){
	//Initialize the lock and the client list, then takes the date of for the server
  	int addrlen = sizeof(address);
	if(pthread_mutex_init(&lock, NULL) != 0){
		printf("\n mutex init failed\n");
    	return 1;
    }
	//zero is the base for all clients, i copi it on all the places of the array so the memory is set and not something random
	struct client zero;
	strcpy(zero.username,"");
	zero.user_id=-1;
	zero.created=0;
	zero.is_first=1;
	zero.number=0;
	for(int i=0;i<10;i++){
		list[i]= zero;
	}

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(date,"%d-%02d-%02d_%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);



	while(1){
		
		for(int i = 0; i < MAX_CLIENTS; i++){
			recvfrom(sockfd, &recividos[i], sizeof(recividos[i]), 0,(struct sockaddr *)&address,&addrlen);
			int *number = malloc(sizeof(*number));
			*number = i;
			addresses[i]= address;
			if(pthread_create(&threads[i], NULL, thread_server,(void *)number)!=0){
				perror("	thread error");
				exit(EXIT_FAILURE);
			}
		}

		for(int i = 0; i < MAX_CLIENTS; i++){
			if(pthread_join(threads[i], NULL)!=0){
				perror("	thread error");
				exit(EXIT_FAILURE);
			}
		}
		//I use n to know how many times the bucle have been executed
		n++;
		printf("------------------%d------------------\n",n);
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