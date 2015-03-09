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
  char msj[bytes];
  char msj1[bytes];
  char msj2[bytes];
  vector<string> ret;
  read(fd,msj,bytes);
  int sz = strlen(msj);
  int ind;
  for(int ind=0;ind<sz;ind++){
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
