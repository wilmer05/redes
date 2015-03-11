#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<vector>
#include<string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

int leer_aux(int fd){
  char buf[5];
  read(fd, buf, 4);
  return atoi(buf);
}

vector<string> leer_comando(int bytes, int fd){
  char msj[bytes+20];
  memset(msj,0,sizeof msj);
  char msj1[bytes+20];
  char msj2[bytes+20];
  memset(msj1,0,sizeof msj1);
  memset(msj2,0,sizeof msj2);
  vector<string> ret;
  read(fd,msj,bytes);
  int sz = strlen(msj);
  int ind;
  for(ind=0;ind<sz;ind++){
    if(msj[ind]==' ') 
      break;
  }
  msj[ind]='\0';
  strcpy(msj1,msj);
  strcpy(msj2,msj+ind+1);
  ret.push_back(string(msj1));
  ret.push_back(string(msj2));
  return ret;
}

void salir(const char *mensaje){
  printf(mensaje);
  exit(1);
}

void escribir_comando(int socket, char *mensaje){
  
 
  char val[5]; 
  memset(val,0,sizeof(val));
  int sz =strlen(mensaje);
  sprintf(val,"%d",sz);
  int err=0;
  err=write(socket,val,4);
  //printf("%d\n",err);
  if(err<0)
    salir("Error al escribir en socket");
  err = write(socket,mensaje,sz);
  if(err<0)
    salir("Error al escribir en socket");
}
