#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>


#define LISTENQ 10
#define MAXDATASIZE 3000
#define MAX_MUSICAS 300
#define CLI_STRING "Cliente, escolha uma string:"
#define CLI_INT "Cliente, escolha um int:"

typedef struct Musica {
    int id;
    char titulo[100];
    char interprete[100];
    char idioma[50];
    char genero[50];
    int ano;
    char refrao[MAXDATASIZE];
} Musica;

#endif /* SERVER_H */
