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

int main(int argc, char **argv) {
    int    sockfd, n;
    char   recvline[MAXLINE + 1];
    char   error[MAXLINE + 1];
    struct sockaddr_in servaddr;

    if (argc != 3) {
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error," <IPaddress>, <Port>");
        perror(error);
        exit(1);
    }

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }
    // Adicionei isto para obter a porta dinâmica
    struct sockaddr_in localAddr;
    socklen_t addrLen = sizeof(localAddr);
    getsockname(sockfd, (struct sockaddr*)&localAddr, &addrLen);
    printf("Cliente conectado à porta local: %d\n", ntohs(localAddr.sin_port));

    while (1) {
        sleep(1);
        // Ler o menu inicial do servidor
        if ((n = read(sockfd, recvline, MAXLINE)) <= 0) {
            if (n == 0) {
                printf("Conexão encerrada pelo servidor.\n");
            } else {
                perror("read error");
            }
            exit(1);
        }
        recvline[n] = '\0';
        if (strstr(recvline, "Cliente, escolha um int:") != NULL) {
            int choice;
            scanf("%d", &choice);

            // Enviar os dados da nova música para o servidor
            write(sockfd, &choice, sizeof(choice));
            // Limpar o buffer de entrada
            while (getchar() != '\n');
        }else if (strstr(recvline, "Cliente, escolha uma string:") != NULL){
            char choice[50];
            scanf("%s", choice);

            // Enviar escolha para o servidor
            write(sockfd, choice, sizeof(choice));
            // Limpar o buffer de entrada
            while (getchar() != '\n');
        }else{
            printf("%s", recvline);
        }

        if (strstr(recvline, "Você escolheu a opção quit.\n") != NULL){
            break;
        }
    }

    if (n < 0) {
        perror("read error");
        exit(1);
    }

    exit(0);
}
