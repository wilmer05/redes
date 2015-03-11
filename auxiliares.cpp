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


/*Funcion que se encarga de leer los bytes auxiliares de mensajes
*
* @param fd es el descriptor de archivo de donde se va a leer los bytes 
*           auxiliares
*
* @return la cantidad de bytes que han de leerse despues, para leer por completo
*         el mensaje
*/
int leer_aux(int fd){
  char buf[5];
  read(fd, buf, 4);
  return atoi(buf);
}


/*Funcion encargada de leer un comando enviado por el usuario
*
*@param bytes entero que indica la cantidad de bytes a leer
*@param fd    entero queindica elsocket por el que se va a leer
*
*@return      un vector de string que indica cual es el comando y su argumento
*/
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



/*Funcion auxiliar utilizada para terminar un proceso
*
*@param mensaje es una cadena de caracteres que indica que mensaje se va a dar
*       para terminar el programa
*
*/
void salir(const char *mensaje){
  printf(mensaje);
  exit(1);
}


/*Funcion utilizada para escribir un mensaje cualquiera en un socket
* pasado como argumento
*
*@param socket   es el socket en el que se desea escribir
*@param mensaje  es el mensaje que se desea escribir en el socket
*
*
*/
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
