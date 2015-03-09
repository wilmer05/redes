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


//Funcion que se encarga de leer los bytes auxiliares de mensajes
int leer_aux(int );


//Funcion encargada de leer un comando enviada por el usuario
vector<string> leer_comando(int , int );


//funcion auxiliar utilizada para terminar un proceso
void salir(const char *);


void escribir_comando(int , char *);
