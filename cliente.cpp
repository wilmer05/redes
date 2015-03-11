#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <set>
#include <netdb.h>
#include "auxiliares.h"
#include <signal.h>

using namespace std;

const int kMaxComando = 2000;
char comando[kMaxComando];
char buffer[kMaxComando];


/* Funcion encargada de manejar la senial SIGPIPE
*
*  @param x es el error que dio
*/
void senial_pipe(int x){
    printf("El socket se desconecto\n",x);
    printf("Saliendo\n");
    exit(1);
}

/* Funcion encargada de manejarel hilo de un cliente para la lectura de
*  los mensajes que le llegan por un socket
*
*  @param arg   es un apuntador que luego de un casteo apunta a un entero que es 
*               el socket por el que se va a leer
*
*/
void *hilo_cliente(void *arg){
  int fd = *((int*)arg);

  while(1){
    memset(buffer,0,sizeof buffer);
    int bytes = leer_aux(fd);
    read(fd,buffer,bytes);
    if(!(strcmp(buffer,"salir"))){
      printf("Hasta luego\n");
      break;
    }
    printf("\n");
    printf(buffer);
  }
  
}


/* Funcion que se utiliza para manejar los comandos que un cliente manda
*  
*  @param arg   es un apuntador que luego de un casteo apunta a un entero que es 
*               el socket por el que se va a leer
*/
void *hilo_comandos(void *arg){
  int sockfd = *((int*)arg);   
  printf("Bienvenido al servicio de chat SCS:\n");
  printf("\nIntroduzca un comando: ");
  while(1){
    memset(comando,0,sizeof(comando));
    fgets(comando,kMaxComando,stdin);
    int sz = strlen(comando);  
    comando[sz-1]='\0';
    sz--;   

    if(sz!=0){
      escribir_comando(sockfd,comando);
    }
    if(!(strcmp(comando,"salir")))
      break; 
      
  }
}


/*  Funcion principal encargada de realizar la conexion con el servidor
*   y de iniciar los hilos
*
*   @param argc   Cantidad de argumentos que el usuario le paso al programa
*   @param argv   Argumentos que el usuarios le paso al programa
*
*   @return       un numero que indica si el programa termino correctamente
*/
int main(int argc, char *argv[]){

  if(argc!=5 || strcmp("-d",argv[1]) || strcmp("-p",argv[3]) || !atoi(argv[4])){
    if(argc!=7 || strcmp("-d",argv[1]) || strcmp("-p",argv[3]) || \
                !atoi(argv[4]) || strcmp("-l",argv[5]) || !atoi(argv[6])){
      salir("Argumentos invalidos:\n" \
      "Uso:\n scs_cli -d <nombre_módulo_atención> -p <puerto_scs_svr>"\
      " [-l<puerto_local>]\n");  
    }
  }
  
  char *host = argv[2];
  char *puerto;
  char *puerto_local=NULL;
  puerto = argv[4];
  if(argc==7)
    puerto_local = argv[6];

  //socket principal
  int sockfd;
  
  //estructuras auxiliares para conexion
  struct sockaddr_in dir_server;
  struct in_addr addr;
  struct hostent *server;
  int newsock;
  
  
  //nueva funcion para la seniales de los pipes
  signal(SIGPIPE,senial_pipe);

  bzero(&dir_server,sizeof(dir_server));
  dir_server.sin_family = AF_INET;
  if (inet_aton(host, &(dir_server.sin_addr)))
      server = gethostbyaddr((char *) &(dir_server.sin_addr), sizeof(dir_server.sin_addr), AF_INET);
  else
      server = gethostbyname(host);
  

  dir_server.sin_port = htons(atoi(puerto));
  
  printf("Realizando conexion...\n");
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd <0){
    salir("No se ha podido abrir un socket");
  }

  if (connect(sockfd, (struct sockaddr *) &dir_server,
              sizeof(dir_server)) < 0){
        salir("Ha fallado la conexion con el server.");
  }  
  pthread_t id,id2;
  if(pthread_create(&id, NULL, hilo_cliente,\
                                      (void *) (& sockfd))){
    salir("No se ha podido realizar hilos\n");
  }
  printf("Conectado\n");
 
  if(pthread_create(&id2, NULL, hilo_comandos,\
                                      (void *) (& sockfd))){
    salir("No se ha podido realizar hilos\n");
  }
  
  pthread_join(id,NULL);
  pthread_join(id2,NULL);

  return 0; 
}
