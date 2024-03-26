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
#include <unistd.h>

#define MAXLINE 4096

typedef struct Musica {
    int id;
    char titulo[100];
    char interprete[100];
    char idioma[50];
    char genero[50];
    int ano;
} Musica;

void interagir_com_servidor(int sockfd);

#endif /* CLIENT_H */
