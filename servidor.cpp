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

using namespace std;

pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutes_salas = PTHREAD_MUTEX_INITIALIZER;

//cantidad maxima de elementos a esperar en la cola del socket
const int kColaSocket = 50;

struct usuario{
  string nombre;
  string sala;
  int fd;
  pthread_t id;
  int num;
  usuario(string name,string room, int sock,int n):nombre(name),sala(room),\
                                                fd(sock),num(n){
  }
  
};

struct sala{
  string nombre;
  bool habilitada;
  sala(string s):nombre(s){
    habilitada=true;
  }
  bool operator<(const sala &ot) const{
    return nombre<ot.nombre;
  }
};

vector<usuario> users;
set<sala> rooms;
set<string> usuarios_validos;

char *bitacora;


void procesar_comando(string &s1, string &s2, int fd, int num_user){
  
  
}

void *hilo(void *user){
  usuario *ptr = (usuario *)user;
  pthread_t id = ptr->id;
  int fd = ptr->fd; 
  //leo las peticiones del usuario
  while(1){
    int bytes = leer_aux(fd);
    vector<string> leido = leer_comando(bytes,fd);
    if(leido[0]=="salir") 
      break;
    
    procesar_comando(leido[0],leido[1],fd,ptr->num);
    
  }
  
  pthread_cancel(id);

}

int main(int argc, char *argv[]){

  if(argc!=5 || strcmp("-l",argv[1]) || strcmp("-b",argv[3]) || !atoi(argv[2])){
    salir("Argumentos invalidos:\n" \
    "Uso:\n ./scs_svr -l <puerto_scs_svr> -b <archivo_bitácora>\n");  
  }
  char *puerto;
  puerto = argv[2];
  bitacora = argv[4];
  //printf("aqui1");
  //socket principal
  int sockfd;
  struct sockaddr_in server_info,cliente_info;
  int newsock;
  //printf("aqui1");
    
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
    salir("No se ha podido abrir un socket");
  }
  memset(&server_info,0,sizeof(server_info));
  server_info.sin_family = AF_INET;
  server_info.sin_addr.s_addr = htonl(INADDR_ANY);
  server_info.sin_port = htons(atoi(puerto));
  //printf("aqui1");
  if(bind(sockfd,(struct sockaddr *) &server_info,sizeof(server_info))){
    salir("No pudo hacer el link del socket\n");
  }
    
//    printf("aqui1");
  if(listen(sockfd,kColaSocket)<0){
    salir("El socket no pudo escuchar\n");
  }
  usuarios_validos.insert("root");
  //loop que acepta conexiones
  int tam = sizeof(cliente_info);
  while(1){
//    printf("oyendo");
    newsock = accept(sockfd,(struct sockaddr *) &cliente_info,(socklen_t *)&tam);
    if(newsock<0)
      salir("El servidor ha fallado al aceptar conexiones\n");
      pthread_mutex_lock(&mutex_usuarios);
      int sz = users.size();
      users.push_back(usuario("-1","-1",newsock,sz));
      sz = users.size();
      if(pthread_create(&(users[sz-1].id), NULL, hilo,\
                                      (void *) (&users[sz-1]))){
        salir("El servidor no ha podido realizar hilos\n");
      }
      pthread_mutex_unlock(&mutex_usuarios);
  
  }
  

  return 0; 
}
