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
#include<iostream>
#include <netdb.h>
#include <time.h>
#include "auxiliares.h"

using namespace std;


//Mutex utilizados para la memoria compartida
pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_salas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitac = PTHREAD_MUTEX_INITIALIZER;

//Cantidad maxima de elementos a esperar en la cola del socket
const int kColaSocket = 50;
const int kTamBuf = 5010;


//Estructura de usuario donde se tiene toda la informacion correspondiente
//a un usuario que se conecto
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


//Estructura de datos encargada de guardar toda la informacion de una sala
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


//vectores y set globales que el servidor maneja para pasar la informacion de un
//hilo a otro por el que se comunican los usuarios
vector<usuario> users;
set<sala> rooms;
set<string> usuarios_validos;
set<sala>::iterator sit;
set<string>::iterator user_it;


//apuntador al nombre de la bitacora
char *bitacora;


/*  Funcion encargada de buscar el nombre de usuario que maneja un socket dado
*
*   @param fd   es el socket al que le deseamos buscar el usuario manejado
*   
*   @return     un string que es el nombre de usuario buscado
*/
string buscar_nombre(int fd){
  pthread_mutex_lock(&mutex_usuarios);
  string ret = "";
  for(int i=0;i<users.size();i++){
    if(users[i].fd == fd)
      ret = users[i].nombre;
  }
  pthread_mutex_unlock(&mutex_usuarios);
  return ret;
}

/*  Funcion encargada de retornar la hora actual del sistema
*
*   @return     un apuntador a char con la hora dada
*/
char *getTime(){
  time_t current_time;
  char* c_time_string;
  current_time = time(NULL);
  c_time_string = ctime(&current_time);
  return c_time_string;
}

/*  Funcion encargada de buscar el numero en el arreglo de usuarios de un 
*   usuario que maneja un socket dado
*
*   @param fd   es el socket al que le deseamos buscar su numero en el arreglo
*   
*   @return     un entero que es su posicion en el arreglo
*/
int buscar_num(int fd){
  pthread_mutex_lock(&mutex_usuarios);
  int ret = 0;
  for(int i=0;i<users.size();i++){
    if(users[i].fd == fd)
      ret = users[i].num;
  }
  pthread_mutex_unlock(&mutex_usuarios);
  return ret;
}


/*  Funcion encargada de buscar la sala de un usuario que maneja un socket dado
*
*   @param fd   es el socket al que le deseamos buscar la sala manejada
*   
*   @return     un string que es la sala del usuario buscado
*/
string buscar_sala(int fd){
  pthread_mutex_lock(&mutex_usuarios);
  string ret="";
  for(int i=0;i<users.size();i++){
    if(users[i].fd == fd){
      ret=users[i].sala;
    }
  }
  pthread_mutex_unlock(&mutex_usuarios);
  return ret;
}


/*  Funcion encargada realizar un broadcast a todos los usuarios en una sala 
*   dada
*
*   @param sock   es el socket que desea enviar el mensaje
*   @param sal  es el nombre de la sala por el que se enviara el broadcast
*   @param msj  es el mensaje que se desea transmitir
*   @usr        es el nombre del usuario que enviara el mensaje
*   
*/
void broadcast(int sock,string sal, string msj,string usr){
  pthread_mutex_lock(&mutex_usuarios);

  if(sal=="-1") 
    return;

  string ret=usr+": "+msj;
  
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  strcpy(buf,msj.c_str());

  for(int i=0;i<users.size();i++){
    if(users[i].sala==sal && users[i].nombre!="-1"){
      escribir_comando(users[i].fd,buf);
    }
  }
  pthread_mutex_unlock(&mutex_usuarios);  
}

/*  Funcion utilizada para ver los usuarios conectados en una sala
*
*   @param sock   es el socket que envio la solicitud
*   @param sal    es el nombre de la sala que se desea ver que usuarios
*                 estan conectadasa ella
*   
*/
void ver_usuarios_sala(int sock,string sal){
  pthread_mutex_lock(&mutex_usuarios);
  string ret="Usuarios en la sala "+sal+":\n";
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  for(int i=0;i<users.size();i++){
    if(users[i].sala==sal && users[i].nombre!="-1"){
      ret+= users[i].nombre;
      ret+="\n";
    }
  }
  ret+="\n==================================\n";
  strcpy(buf,ret.c_str());
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_usuarios);  
}

/*  Funcion utilizada para ver los usuarios existentes en el sistema
*
*   @param sock   es el socket que envio la solicitud
*   
*/
void ver_usuarios(int sock){

  pthread_mutex_lock(&mutex_usuarios);
  string ret="Usuarios validos en el sistema:\n";
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  for(user_it=usuarios_validos.begin();user_it!=usuarios_validos.end();++user_it){
    ret+= *user_it;
    ret+="\n";
  }
  ret+="\n==================================\n";
  strcpy(buf,ret.c_str());
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_usuarios);  
}


/*  Funcion utilizada para ver las salas existentes en el sistema
*
*   @param sock   es el socket que envio la solicitud
*   
*/
void ver_salas(int sock){
  pthread_mutex_lock(&mutex_salas);
  string ret="Salas:\n";
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  for(sit=rooms.begin();sit!=rooms.end();++sit){
    ret+= sit->nombre;
    ret+=" ----> ";
    if(!(sit->habilitada)){
      ret+="no";
    }
    ret+=" esta habilitada";
    ret+="\n";
  }
  ret+="\n==================================\n";
  strcpy(buf,ret.c_str());
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);  
}

/*  Funcion utilizada para que un usuario dado un socket, deje la sala
*
*   @param sock   es el socket que envio la solicitud
*   @param usr    es el numero en el arreglo de usuarios del usuario solicitante
*   
*/
void dejar_sala(int sock, int usr){
  pthread_mutex_lock(&mutex_usuarios);
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  users[usr].sala="-1";
  strcpy(buf,"Has dejado la sala\n");
  escribir_comando(sock,buf);
  
  pthread_mutex_unlock(&mutex_usuarios);
}

/*  Funcion utilizada para entrar en una sala dada
*
*   @param sock   es el socket que envio la solicitud
*   @param entrar nombre de la sala a la cual el usuario quiere entrar
*   @param usr    es el numero en el arreglo de usuarios del usuario solicitante
*   
*/
void entrar_en_sala(int sock,string entrar, int usr){
  
  pthread_mutex_lock(&mutex_usuarios);  
  pthread_mutex_lock(&mutex_salas);
  sala s(entrar);

  
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  if(!rooms.count(s)){
    strcpy(buf,"Error, sala inexistente\n");
  }
  else{
    sit = rooms.find(s);
    if(sit->habilitada){
      users[usr].sala=entrar;
      strcpy(buf,"Has entrado en la sala\n");
    }
    else{
      strcpy(buf,"Sala deshabilitada\n");
    }
  }
  
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);
  pthread_mutex_unlock(&mutex_usuarios);    
}


/*  Funcion utilizada para conectarse con un usuario dado
*
*   @param sock   es el socket que envio la solicitud
*   @param entrar nombre del usuario con el que se desea iniciar sesion
*   @param usr    es el numero en el arreglo de usuarios del usuario solicitante
*   
*/
void conexion(int sock, string name, int usr){
  pthread_mutex_lock(&mutex_usuarios);    
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  if(!usuarios_validos.count(name)){
    strcpy(buf,"Usuario invalido\n");
  }
  else{
    int c =0;
    for(int i=0;i<users.size();i++){
      if(users[i].nombre==name){
        c++;
      }
    }
    if(c){
      strcpy(buf,"Ya hay un usuario con ese nombre\n");
    }
    else{
      strcpy(buf,"Loggin exitoso\n");
      users[usr].nombre=name;
    }
  }
  
  escribir_comando(sock,buf);  
  pthread_mutex_unlock(&mutex_usuarios);  
}


/*  Funcion utilizada por el usuario root del sistema para crear un usuario
*
*   @param sock     es el socket que envio la solicitud
*   @param entrar   nombre del usuario a agregar en la sala
*   @param nombre   nombre del usuario que hizo la solicitud o comando, debe
*                   ser root para utilizar este comando
*   
*/
void crear_usuario(int sock, string &name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);  
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));

  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(usuarios_validos.count(name)){
    strcpy(buf,"Ya este usuario existe en el sistema\n");
  }
  else{
    usuarios_validos.insert(name);
    strcpy(buf,"Usuario creado.\n");
  }
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_usuarios);  
}


/*  Funcion utilizada por el usuario root del sistema para eliminar un usuario
*           del sistema
*
*   @param sock     es el socket que envio la solicitud
*   @param name     nombre del usuario a borrar
*   @param nombre   nombre del usuario que hizo la solicitud o comando, debe
*                   ser root para utilizar este comando
*   
*/
void eliminar_usuario(int sock, string name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);  
  char buf[kTamBuf];
  char buf2[kTamBuf];
  
  memset(buf,0,sizeof(buf));
  memset(buf2,0,sizeof(buf2));
  strcpy(buf2,"El usuario que usted estaba utilizando fue borrado, en este\
  momento usted esta conectado sin ningun usuario por lo tanto si estaba\
  en una sala ha sido botado de ella.\n");
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(!usuarios_validos.count(name)){
    strcpy(buf,"Este usuario no existe en el sistema\n");
  }
  else{
    for(int i=0;i<users.size();i++){
      if(users[i].nombre==name){
        users[i].nombre="-1";
        users[i].sala="-1";
        escribir_comando(users[i].fd,buf2);
      }
    }
    rooms.erase(name);
    usuarios_validos.erase(name);
    strcpy(buf,"Usuario borrado con exito.\n");
  }
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_usuarios);  
}


/*  Funcion utilizada por el usuario root del sistema crear una sala
*
*   @param sock     es el socket que envio la solicitud
*   @param name     nombre de la sala a crear
*   @param nombre   nombre del usuario que hizo la solicitud o comando, debe
*                   ser root para utilizar este comando
*   
*/
void crear_sala(int sock, string name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);  
  pthread_mutex_lock(&mutex_salas);  
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  sala s(name);
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(rooms.count(s)){
    strcpy(buf,"Ya esta sala existe en el sistema\n");
  }
  else{
    rooms.insert(s);
    strcpy(buf,"Sala creada.\n");
  }
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);
  pthread_mutex_unlock(&mutex_usuarios);      
}

/*  Funcion utilizada por el usuario root del sistema para eliminar una sala
*           del sistema
*
*   @param sock     es el socket que envio la solicitud
*   @param name     nombre de la sala a borrar
*   @param nombre   nombre del usuario que hizo la solicitud o comando, debe
*                   ser root para utilizar este comando
*   
*/
void borrar_sala(int sock, string name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);      
  pthread_mutex_lock(&mutex_salas);
  char buf[kTamBuf];
  char buf2[kTamBuf];
  memset(buf,0,sizeof(buf));
  memset(buf2,0,sizeof(buf2));
  sala s(name);
  strcpy(buf2,"La sala a la que pertenecias ha sido eliminada \
ahora no estas en ninguna sala\n");
  
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(!rooms.count(s)){
    strcpy(buf,"Esta sala no existe en el sistema\n");
  }
  else{
    for(int i=0;i<users.size();i++){
      if(users[i].sala==name){
        users[i].sala="-1";
        escribir_comando(users[i].fd,buf2);
      }
    }
    rooms.erase(s);
    strcpy(buf,"Usuario borrado con exito.\n");
  }
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);  
  pthread_mutex_unlock(&mutex_usuarios);        
}

/*  Funcion utilizada por el usuario root del sistema para ver los usuarios 
*   conectados a una sala
*
*   @param sock     es el socket que envio la solicitud
*   @param name     nombre de la sala en donde estan los usuarios
*   @param nombre   nombre del usuario que hizo la solicitud o comando, debe
*                   ser root para utilizar este comando
*   
*/
void ver_usuarios_root(int sock,string name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);
  pthread_mutex_lock(&mutex_salas);
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  sala s(name);
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(!rooms.count(s)){
    strcpy(buf,"Esta sala no existe en el sistema\n");
  }
  else{
    string ret = "Usuarios en la sala solicitada:\n";
    for(int i=0;i<users.size();i++){
        if(users[i].sala==name){
          if(users[i].nombre!="-1")
            ret+=users[i].nombre+"\n";
        }
    }
    ret+="\n=================\n";
    strcpy(buf,ret.c_str());
  }
  
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);
  pthread_mutex_unlock(&mutex_usuarios);  
}


/*  Funcion utilizada por el usuario root del sistema para habilitar una sala
*   en el sistema
*
*   @param sock     es el socket que envio la solicitud
*   @param name     nombre de la sala a habilitar
*   @param nombre   nombre del usuario que hizo la solicitud o comando, debe
*                   ser root para utilizar este comando
*   
*/
void habilitar_sala(int sock,string name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);
  pthread_mutex_lock(&mutex_salas);
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  sala s(name);
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(!rooms.count(s)){
    strcpy(buf,"Esta sala no existe en el sistema\n");
  }
  else{
    rooms.erase(s);
    rooms.insert(s);
    
    strcpy(buf,"Sala habilitada.\n");
  }
  
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);
  pthread_mutex_unlock(&mutex_usuarios);  
}

void deshabilitar_sala(int sock,string name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);
  pthread_mutex_lock(&mutex_salas);
  char buf[kTamBuf];
  char buf2[kTamBuf];
  memset(buf,0,sizeof(buf));
  memset(buf2,0,sizeof(buf2));
  sala s(name);
  strcpy(buf2,"La sala a la que pertenecia ha sido deshabilitada, ahora\
  no estas en ninguna sala\n");
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(!rooms.count(s)){
    strcpy(buf,"Esta sala no existe en el sistema\n");
  }
  else{
    sit = rooms.find(s);
    rooms.erase(s);
    s.habilitada=false;
    rooms.insert(s);
    for(int i=0;i<users.size();i++){
      if(users[i].sala==name){
        users[i].sala="-1";
        escribir_comando(users[i].fd,buf2);
      }
    }
    
    strcpy(buf,"Sala deshabilitada.\n");
  }
  
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);
  pthread_mutex_unlock(&mutex_usuarios);  
}

/*  Funcion utilizada por el usuario root del sistema para ver el log
*
*   @param sock     es el socket que envio la solicitud
*   @param nombre   nombre del usuario que hizo la solicitud o comando, debe
*                   ser root para utilizar este comando
*   
*/
void ver_log(int sock, string nombre){
  pthread_mutex_lock(&mutex_usuarios);  
  char buf[10*kTamBuf];
  memset(buf,0,sizeof(buf));
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else{
    pthread_mutex_lock(&mutex_bitac);
    FILE *fp = fopen(bitacora,"r");
    string ret="";
    while(fgets(buf,10*kTamBuf,fp)!=NULL){
      ret+=string(buf);
    }
    fclose(fp);
    strcpy(buf,ret.c_str());
    
    pthread_mutex_unlock(&mutex_bitac);
  }
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_usuarios);  
}



/*  Funcion utilizada para escribir en el log del sistema
*
*   @param sock     es el socket que envio la solicitud
*   @param msj      es el mensaje a escribir en el log es un char *
*   
*/
void escribir_log(int sock, char *msj){
  FILE *fp = fopen(bitacora,"a");
  fprintf(fp,"%s",msj);
  fclose(fp);
}



/*  Funcion utilizada para escribir en el log del sistema
*
*   @param sock     es el socket que envio la solicitud
*   @param msj      es el mensaje a escribir en el log es un const char *
*   
*/
void escribir_log(int sock, const char *msj){
  FILE *fp = fopen(bitacora,"a");
  fprintf(fp,"%s",msj);
  fclose(fp);
}

/*  Funcion utilizada para escribir en el log del sistema
*
*   @param sock     es el socket que envio la solicitud
*   @param msj      es el mensaje a escribir en el log es un string  
*   
*/
void escribir_log(int sock, string msj){
  escribir_log(sock,msj.c_str());
}

/*  Funcion utilizada para procesar las solicitudes de un cliente
*
*   @param s1     es el comando hecho por el cliente
*   @param s2     es el argumento del comando s1   
*   @param fd     es el socket que envio la solicitud
*   
*/
void procesar_comando(string s1, string s2, int fd){

  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  strcpy(buf,s1.c_str());

//  cout << s1 << " "<< s2 << " " << fd << endl;
  string nombre,sal;
  nombre = buscar_nombre(fd);
  sal = buscar_sala(fd);
  int num_user = buscar_num(fd);
  if(s1=="salir"){
    pthread_mutex_lock(&mutex_usuarios);
    escribir_comando(fd,buf);
    users[num_user].nombre="-1";
    users[num_user].sala = "-1";
    pthread_mutex_unlock(&mutex_usuarios);
  }
  else if(s1=="conectarse"){
    conexion(fd,s2,num_user);
  }
  else if(s1=="entrar"){
    entrar_en_sala(fd,s2,num_user);
  }
  else if(s1=="dejar"){
    dejar_sala(fd,num_user);
  }
  else if(s1=="ver_salas"){
    ver_salas(fd);
  }
  else if(s1=="ver_usuarios"){
    if(s2==""){
      ver_usuarios(fd);
    }
    else{
      ver_usuarios_root(fd,s2,nombre);
    }
  }
  else if(s1=="ver_usu_salas"){
    ver_usuarios_sala(fd,s2);
  }
  else if(s1=="env_mensaje"){
    s2+="\n";
    broadcast(fd,sal,s2,nombre);
  }
  else if(s1=="crear_usu"){
    crear_usuario(fd,s2,nombre);
  }
  else if(s1=="elim_usu"){
    eliminar_usuario(fd,s2,nombre);
  }
  else if(s1=="crear_sala"){
    crear_sala(fd,s2,nombre);
  }
  else if(s1=="elim_sala"){
    borrar_sala(fd,s2,nombre);
  }
  else if(s1=="hab_sala"){
    habilitar_sala(fd,s2,nombre);
  }
  else if(s1=="deshab_sala"){
    deshabilitar_sala(fd,s2,nombre);  
  }  
  else if(s1=="ver_log"){
    ver_log(fd,nombre);
  }
  else{
    strcpy(buf,"Comando invalido\n");
    escribir_comando(fd,buf);
  }
  if(s1!="salir"){
    memset(buf,0,sizeof(buf));
    strcpy(buf,"\nIntroduzca un comando: ");
    escribir_comando(fd,buf);  
  }
  printf("Ok\n");
}


/*  Funcion encargada del hilo de cada usuario y de leer los comandos que el
*   cliente envia
*
*   @param user   es un apuntador que apunta a una estructura usuario
*   
*/
void *hilo(void *user){
  usuario *ptr = (usuario *)user;
  pthread_t id = ptr->id;
  int fd = ptr->fd; 
  //leo las peticiones del usuario
  
  while(1){
    int bytes = leer_aux(fd);
    vector<string> leido = leer_comando(bytes,fd);

    procesar_comando(leido[0],leido[1],fd);
    
    if(leido[0]=="salir") 
      break;
    
  }
  
  pthread_cancel(id);

}

/*  Funcion principal encargada de abrir el socket que escucha conexiones y 
*   mantenerse escuchando los usuarios que quieren iniciar una conexion con el
*   servidor. Tambien se encarga de la creacion de los hilos
*
*   @param argc   Cantidad de argumentos que el usuario le paso al programa
*   @param argv   Argumentos que el usuarios le paso al programa
*
*   @return       un numero que indica si el programa termino correctamente
*   
*/
int main(int argc, char *argv[]){

  if(argc!=5 || strcmp("-l",argv[1]) || strcmp("-b",argv[3]) || !atoi(argv[2])){
    salir("Argumentos invalidos:\n" \
    "Uso:\n ./scs_svr -l <puerto_scs_svr> -b <archivo_bitácora>\n");  
  }
  char *puerto;
  puerto = argv[2];
  bitacora = argv[4];
  FILE *fp = fopen(bitacora,"a");
  fclose(fp);

  int sockfd;
  struct sockaddr_in server_info,cliente_info;
  int newsock;

  printf("Iniciando servidor...\n");    
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
    salir("No se ha podido abrir un socket");
  }
  memset(&server_info,0,sizeof(server_info));
  server_info.sin_family = AF_INET;
  server_info.sin_addr.s_addr = htonl(INADDR_ANY);
  server_info.sin_port = htons(atoi(puerto));
  printf("Creando socket...\n");    
  if(bind(sockfd,(struct sockaddr *) &server_info,sizeof(server_info))){
    salir("No pudo hacer el link del socket\n");
  }
    
  printf("Abriendo socket para escuchar conexiones...\n");    
  if(listen(sockfd,kColaSocket)<0){
    salir("El socket no pudo escuchar\n");
  }
  usuarios_validos.insert("root");

  int tam = sizeof(cliente_info);
  
  printf("Servidor levantado.\n");    
  //loop que acepta conexiones
  while(1){

    newsock = accept(sockfd,(struct sockaddr *) &cliente_info,(socklen_t *)&tam);

    if(newsock<0)
      salir("El servidor ha fallado al aceptar conexiones\n");
      pthread_mutex_lock(&mutex_usuarios);
      int sz = users.size();
      users.push_back(usuario("-1","-1",newsock,sz));
      sz = users.size();
      pthread_mutex_unlock(&mutex_usuarios);
      if(pthread_create(&(users[sz-1].id), NULL, hilo,\
                                      (void *) (&users[sz-1]))){
        salir("El servidor no ha podido realizar hilos\n");
      }
      
  
  }
  

  return 0; 
}
