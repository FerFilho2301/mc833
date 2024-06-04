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
#include <pthread.h>


#define LISTENQ 10
#define MAXDATASIZE 2000
#define UDP_PORT 8080
#define MAX_MUSICAS 300
#define EOF_MESSAGE "EOF" 
#define TIMEOUT_SEC 6
#define MAX_PACKETS 4000
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

typedef struct {
    int seq_num;
    char data[MAXDATASIZE];
    int length;
} Packet;

//Caminho inverso para envio de músicas
void receive_udp_file(const char* local_filename) {
    int udp_sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAXDATASIZE];
    ssize_t bytesRead;

    // Criando um socket UDP
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) {
        perror("Não foi possível criar o socket.");
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(UDP_PORT);

    if (bind(udp_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind falhou.");
        close(udp_sockfd);
        return;
    }

    FILE *file = fopen(local_filename, "wb");
    if (file == NULL) {
        perror("Abrir o arquivo falhou.");
        close(udp_sockfd);
        return;
    }

    Packet packets[MAX_PACKETS];
    memset(packets, 0, sizeof(packets));
    int expected_seq_num = 0;

    fd_set readfds;
    struct timeval tv;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(udp_sockfd, &readfds);

        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;

        // Esperando dados ou o timeout
        int rv = select(udp_sockfd + 1, &readfds, NULL, NULL, &tv);
        if (rv == -1) {
            perror("select error");
            break;
        } else if (rv == 0) {
            printf("Download concluído!\n");
            break; // Timeout ocorreu, quebrar o loop
        }

        bytesRead = recvfrom(udp_sockfd, buffer, MAXDATASIZE, 0, NULL, NULL);
        if (bytesRead <= 0) break; // Sair do loop se ocorrer algum erro ou não houverem dados

        int received_seq_num;
        memcpy(&received_seq_num, buffer, sizeof(int));

        if (bytesRead == sizeof(int) + strlen(EOF_MESSAGE) + 1 && strncmp(buffer + sizeof(int), EOF_MESSAGE, strlen(EOF_MESSAGE) + 1) == 0) {
            break; // Checar se recebe mensagem de EOF
        }

        // Checar se o pacote tem o numero correto
        if (received_seq_num == expected_seq_num) {
            fwrite(buffer + sizeof(int), 1, bytesRead - sizeof(int), file);
            expected_seq_num++;

            // Checar o buffer por pacotes não esperados
            while (packets[expected_seq_num % MAX_PACKETS].length != 0) {
                Packet *pkt = &packets[expected_seq_num % MAX_PACKETS];
                fwrite(pkt->data, 1, pkt->length, file);
                pkt->length = 0; // Marcar como lido
                expected_seq_num++;
            }
        } else if (received_seq_num > expected_seq_num && received_seq_num - expected_seq_num < MAX_PACKETS) {
            
            Packet *pkt = &packets[received_seq_num % MAX_PACKETS];
            memcpy(pkt->data, buffer + sizeof(int), bytesRead - sizeof(int));
            pkt->seq_num = received_seq_num;
            pkt->length = bytesRead - sizeof(int);
        }
    }

    fclose(file);
    close(udp_sockfd);
    printf("Música baixada com sucesso. Clique enter.\n");
}

#endif /* SERVER_H */
