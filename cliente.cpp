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

void ignore_sigpipe(int x){
    printf("Error por socket, socket desconectado\n");
    printf("Saliendo\n");
    exit(1);
}

void *hilo_cliente(void *arg){
  int fd = *((int*)arg);
  //printf("aqui");
  while(1){
    int bytes = leer_aux(fd);
    read(fd,buffer,bytes);
    if(!(strcmp(buffer,"salir"))){
      printf("Hasta luego\n");
      break;
    }
    printf(buffer);
  }
  
}

int main(int argc, char *argv[]){

  if(argc!=5 || strcmp("-d",argv[1]) || strcmp("-p",argv[3]) || !atoi(argv[4])){
    if(argc!=7 || strcmp("-d",argv[1]) || strcmp("-p",argv[3]) || \
                !atoi(argv[4]) || strcmp("-l",argv[5]) || !atoi(argv[6])){
      salir("Argumentos invalidos:\n" \
      "Uso:\n scs_cli -d <nombre_módulo_atención> -p <puerto_scs_svr>"\
      " [-l<puerto_local>]\n");  
    }
  }
  signal(SIGPIPE,ignore_sigpipe);
  char *host = argv[2];
  char *puerto;
  char *puerto_local=NULL;
  puerto = argv[4];
  if(argc==7)
    puerto_local = argv[6];
  //printf("aqui1");
  //socket principal
  int sockfd;
  struct sockaddr_in dir_server;
  struct in_addr addr;
  struct hostent *server;
  int newsock;
  
  if (inet_aton(host, &addr))
      server = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET);
  else
      server = gethostbyname(host);
  
  memset(&dir_server,0,sizeof(dir_server));
  
  memcpy(&dir_server.sin_addr, server->h_addr_list[0], 
           sizeof(dir_server.sin_addr));

  dir_server.sin_port = htons(atoi(puerto));
    
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
    salir("No se ha podido abrir un socket");
  }
  
  if (connect(sockfd, (struct sockaddr *) &dir_server,
              sizeof(dir_server)) < 0){
        salir("Ha fallado la conexion con el server.");
  }  
  pthread_t id;
  if(pthread_create(&id, NULL, hilo_cliente,\
                                      (void *) (& sockfd))){
    salir("No se ha podido realizar hilos\n");
  }

 
  while(1){
   
    //scanf("%[^\n]\n",comando);
    fgets(comando,kMaxComando,stdin);
    int sz = strlen(comando);  
    comando[sz-1]='\0';
    sz--;
    if(!(strcmp(comando,"salir")))
      break;    
    if(sz!=0){
      escribir_comando(sockfd,comando);
      
    }
      
  }
  salir("aja");
  
  pthread_join(id,NULL);


  return 0; 
}
