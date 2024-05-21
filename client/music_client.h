#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h> // For struct timeval
#include <unistd.h>   // For close()

#define MAXLINE 4096
#define UDP_PORT 8080 
#define MAXDATASIZE 3000
#define TIMEOUT_SEC 4
#define EOF_MESSAGE "EOF"
#define MAX_PACKETS 3000  // Maximum number of out-of-order packets we expect to handle

typedef struct Musica {
    int id;
    char titulo[100];
    char interprete[100];
    char idioma[50];
    char genero[50];
    int ano;
} Musica;

typedef struct {
    int seq_num;
    char data[MAXDATASIZE];
    int length;
} Packet;

void interagir_com_servidor(int sockfd);

#endif /* CLIENT_H */
