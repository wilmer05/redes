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
#include "auxiliares.h"

using namespace std;

pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_salas = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitac = PTHREAD_MUTEX_INITIALIZER;

//cantidad maxima de elementos a esperar en la cola del socket
const int kColaSocket = 50;
const int kTamBuf = 5010;

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
set<sala>::iterator sit;
set<string>::iterator user_it;

char *bitacora;

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

void broadcast(int sock,string sal, string msj,string usr){
  pthread_mutex_lock(&mutex_usuarios);

  if(sal=="-1") 
    return;

  string ret=usr+": "+msj;
  
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  strcpy(buf,msj.c_str());
  printf(sal.c_str());
  for(int i=0;i<users.size();i++){
    if(users[i].sala==sal && users[i].nombre!="-1"){
      escribir_comando(users[i].fd,buf);
    }
  }
  pthread_mutex_unlock(&mutex_usuarios);  
}

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
  strcpy(buf,ret.c_str());
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_usuarios);  
}

void ver_usuarios(int sock){

  pthread_mutex_lock(&mutex_usuarios);
  string ret="Usuarios validos:\n";
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  for(user_it=usuarios_validos.begin();user_it!=usuarios_validos.end();++user_it){
    ret+= *user_it;
    ret+="\n";
  }
  strcpy(buf,ret.c_str());
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_usuarios);  
}


void ver_salas(int sock){
  pthread_mutex_lock(&mutex_salas);
  string ret="Salas:\n";
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  for(sit=rooms.begin();sit!=rooms.end();++sit){
    ret+= sit->nombre;
    if(!(sit->habilitada)){
      ret+=" no";
    }
    ret+=" esta habilitada";
    ret+="\n";
  }
  strcpy(buf,ret.c_str());
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);  
}

void dejar_sala(int sock, int usr, string sal){
  pthread_mutex_lock(&mutex_usuarios);
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  users[usr].sala="-1";
  strcpy(buf,"Has dejado la sala\n");
  escribir_comando(sock,buf);
  
  pthread_mutex_unlock(&mutex_usuarios);
}

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

void conexion(int sock, string &name, int usr){
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

void crear_usuario(int sock, string &name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);  
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  printf("creando\n" );;
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

void eliminar_usuario(int sock, string &name, string &nombre){
  pthread_mutex_lock(&mutex_usuarios);  
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(!usuarios_validos.count(name)){
    strcpy(buf,"Este usuario no existe en el sistema\n");
  }
  else{
    usuarios_validos.erase(name);
    strcpy(buf,"Usuario borrado con exito.\n");
  }
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_usuarios);  
}

void crear_sala(int sock, string &name, string &nombre){
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

void borrar_sala(int sock, string &name, string &nombre){
  pthread_mutex_lock(&mutex_usuarios);      
  pthread_mutex_lock(&mutex_salas);
  char buf[kTamBuf];
  char buf2[kTamBuf];
  memset(buf,0,sizeof(buf));
  memset(buf2,0,sizeof(buf2));
  strcpy(buf2,"La sala a la que pertenecias ha sido eliminada \
ahora no estas en ninguna sala\n");
  
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else if(!rooms.count(name)){
    strcpy(buf,"Esta sala no existe en el sistema\n");
  }
  else{
    for(int i=0;i<users.size();i++){
      if(users[i].sala==name){
        users[i].sala="-1";
        escribir_comando(users[i].fd,buf2);
      }
    }
    rooms.erase(name);
    strcpy(buf,"Usuario borrado con exito.\n");
  }
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);  
  pthread_mutex_unlock(&mutex_usuarios);        
}

void ver_usuarios_root(int sock,string name, string nombre){
  pthread_mutex_lock(&mutex_usuarios);
  pthread_mutex_lock(&mutex_salas);
  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  if(nombre!="root"){
    strcpy(buf,"Debe ser usuario root para ejecutar este comando\n");
  }
  else{
    string ret = "Usuarios en la sala solicitada:\n";
    for(int i=0;i<users.size();i++){
        if(users[i].sala==name){
          if(users[i].nombre!="-1")
            ret+=users[i].nombre+"\n";
        }
    }
    strcpy(buf,ret.c_str());
  }
  
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);
  pthread_mutex_unlock(&mutex_usuarios);  
}

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
    
    strcpy(buf,"Sala habilitada.\n");
  }
  
  escribir_comando(sock,buf);
  pthread_mutex_unlock(&mutex_salas);
  pthread_mutex_unlock(&mutex_usuarios);  
}

void ver_log(int sock, string &nombre){
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

void procesar_comando(string s1, string s2, int fd){

  char buf[kTamBuf];
  memset(buf,0,sizeof(buf));
  strcpy(buf,s1.c_str());
  //printf(" %s %s de %s\n",s1.c_str(),s2.c_str(),nombre.c_str());
  cout << s1 << " "<< s2 << " " << fd << endl;
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
    return;
  }
  else if(s1=="conectarse"){
    conexion(fd,s2,num_user);
  }
  else if(s1=="entrar"){
    entrar_en_sala(fd,s2,num_user);
  }
  else if(s1=="dejar"){
    dejar_sala(fd,num_user,sal);
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
  memset(buf,0,sizeof(buf));
  strcpy(buf,"\nIntroduzca un comando: ");
  escribir_comando(fd,buf);  
  printf("Ya\n");
}

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
  
  if(bind(sockfd,(struct sockaddr *) &server_info,sizeof(server_info))){
    salir("No pudo hacer el link del socket\n");
  }
    
  if(listen(sockfd,kColaSocket)<0){
    salir("El socket no pudo escuchar\n");
  }
  usuarios_validos.insert("root");
  //loop que acepta conexiones
  int tam = sizeof(cliente_info);
  while(1){
  
   // printf("oyendo por %s\n",puerto);

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
