#include "music_client.h"

//Caminho inverso para envio de músicas para o servidor
void send_file_udp(const char* filename, struct sockaddr_in cliaddr) {
    int udp_sockfd;
    char buffer[MAXDATASIZE];
    FILE *file;
    struct stat file_stat;
    off_t total_sent = 0;
    double progress = 0.0;
    int sequence_number = 0;

    // Criar socket UDP
    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Criação do socket UDP falhou.");
        return;
    }

    // Abrir o arquivo
    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Abrir o arquivo falhou.");
        close(udp_sockfd);
        return;
    }

    // Pewgar o tamanho do arquivo
    if (stat(filename, &file_stat) != 0) {
        perror("Pegar o tamanho do arquivo falhou.");
        fclose(file);
        close(udp_sockfd);
        return;
    }

    long long file_size = file_stat.st_size;
    int bytesRead;
    // Adicionar o numero de sequencia a cada pacote
    while ((bytesRead = fread(buffer + sizeof(int), 1, sizeof(buffer) - sizeof(int), file)) > 0) {
        memcpy(buffer, &sequence_number, sizeof(int));
        sendto(udp_sockfd, buffer, bytesRead + sizeof(int), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
        sequence_number++;
        total_sent += bytesRead;
        progress = (double)total_sent / file_size * 100.0;
        printf("Enviado %ld de %lld bytes (%.2f%%), Seq Num: %d\n", total_sent, file_size, progress, sequence_number);
    }

    sleep(1);
    fclose(file);
    close(udp_sockfd);
    sleep(1);
}


// Função para lidar com a interação do cliente com o servidor
void interagir_com_servidor(int sockfd) {
    int    n;
    char   recvline[MAXLINE + 1];

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
        char dummy_buffer[10000];
        sprintf(dummy_buffer, "%s", recvline);
        if (strstr(recvline, "Download iniciado para") != NULL) {
            char local_filename[256];
            char *filename_start = strstr(recvline, "Download iniciado para") + strlen("Download iniciado para") + 1;

            int filename_length = strlen(filename_start) - 1;

            snprintf(local_filename, sizeof(local_filename), "client/data/%.*s.mp3", filename_length, filename_start);
            receive_udp_file(local_filename);
            continue;  
        }

        if (strstr(recvline, "Envie a música") != NULL) {
            char filename[256];

            struct sockaddr_in cliaddr;
            memset(&cliaddr, 0, sizeof(cliaddr));
            cliaddr.sin_family = AF_INET;
            
            //======trecho modificado para especificação do IP do servidor UDP======
            socklen_t cliaddr_len = sizeof(cliaddr);
            // Obter o endereço do servidor
            if (getpeername(sockfd, (struct sockaddr *)&cliaddr, &cliaddr_len) == -1) {
                perror("getpeername");
                return;
            }
            // Converter o endereço IP do servidor para string
            char server_ip[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &(cliaddr.sin_addr), server_ip, INET_ADDRSTRLEN) == NULL) {
                perror("inet_ntop");
                return;
            }
            printf("SERVER IP: %s\n", server_ip);
            cliaddr.sin_addr.s_addr = inet_addr(server_ip);
            //======================================================

            cliaddr.sin_port = htons(UDP_PORT);

            snprintf(filename, sizeof(filename), "client/data/%s.mp3", strstr(recvline, "Envie a música") + strlen("Envie a música") + 1);
            sleep(3);
            send_file_udp(filename, cliaddr);
            continue;  
        }

        if (strstr(recvline, "Cliente, escolha um int:") != NULL) {
            int choice;
            scanf("%d", &choice);

            // Enviar os dados da nova música para o servidor
            write(sockfd, &choice, sizeof(choice));
            // Limpar o buffer de entrada
            while (getchar() != '\n');
        }else if (strstr(recvline, "Cliente, escolha uma string:") != NULL){
            char choice[50];
            scanf("%[^\n]s",choice);

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
}

int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    int    sockfd;
    struct sockaddr_in servaddr;

    // Verificar argumentos de linha de comando
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <IPaddress> <Port>\n", argv[0]);
        exit(1);
    }

    // Criar socket
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    // Conectar ao servidor
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

    // Interagir com o servidor
    interagir_com_servidor(sockfd);

    exit(0);
}