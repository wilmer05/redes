/*
* Chat con cliente-servidor realizado por:
*
* Jonathan Moreno   07-41249
* Catherine Lollett 09-10451
* Adriana D'vera    09-11286
* Wilmer Bandres    10-10055
*
*/

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
#include <time.h>

using namespace std;


/*Funcion que se encarga de leer los bytes auxiliares de mensajes
*
* @param fd es el descriptor de archivo de donde se va a leer los bytes 
*           auxiliares
*
* @return la cantidad de bytes que han de leerse despues, para leer por completo
*         el mensaje
*/
int leer_aux(int fd);


/*Funcion encargada de leer un comando enviado por el usuario
*
*@param bytes entero que indica la cantidad de bytes a leer
*@param fd    entero queindica elsocket por el que se va a leer
*
*@return      un vector de string que indica cual es el comando y su argumento
*/
vector<string> leer_comando(int bytes, int fd);


/*  Funcion encargada de retornar la hora actual del sistema
*
*   @return     un apuntador a char con la hora dada
*/
char *getTime();

/*Funcion auxiliar utilizada para terminar un proceso
*
*@param mensaje es una cadena de caracteres que indica que mensaje se va a dar
*       para terminar el programa
*
*/
void salir(const char *mensaje);


/*Funcion utilizada para escribir un mensaje cualquiera en un socket
* pasado como argumento
*
*@param socket   es el socket en el que se desea escribir
*@param mensaje  es el mensaje que se desea escribir en el socket
*
*
*/
void escribir_comando(int socket, char *mensaje);
