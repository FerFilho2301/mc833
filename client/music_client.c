#include "music_client.h"

void receive_udp_file(const char* local_filename) {
    int udp_sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAXDATASIZE];
    ssize_t bytesRead;

    // Create a UDP socket
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

    // Setup for select
    fd_set readfds;
    struct timeval tv;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(udp_sockfd, &readfds);

        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;

        // Wait for data or a timeout
        int rv = select(udp_sockfd + 1, &readfds, NULL, NULL, &tv);
        if (rv == -1) {
            perror("select error");
            break;
        } else if (rv == 0) {
            printf("Download concluído!\n");
            break; // Timeout occurred, break the loop
        }

        bytesRead = recvfrom(udp_sockfd, buffer, MAXDATASIZE, 0, NULL, NULL);
        if (bytesRead <= 0) break; // Exit loop if error or no data

        int received_seq_num;
        memcpy(&received_seq_num, buffer, sizeof(int));

        if (bytesRead == sizeof(int) + strlen(EOF_MESSAGE) + 1 && strncmp(buffer + sizeof(int), EOF_MESSAGE, strlen(EOF_MESSAGE) + 1) == 0) {
            break; // Check for EOF message
        }

        // Check if the packet is the expected sequence number
        if (received_seq_num == expected_seq_num) {
            fwrite(buffer + sizeof(int), 1, bytesRead - sizeof(int), file); // Write the data part
            expected_seq_num++;

            // Check buffer for the next expected packets
            while (packets[expected_seq_num % MAX_PACKETS].length != 0) {
                Packet *pkt = &packets[expected_seq_num % MAX_PACKETS];
                fwrite(pkt->data, 1, pkt->length, file);
                pkt->length = 0; // Mark as read
                expected_seq_num++;
            }
        } else if (received_seq_num > expected_seq_num && received_seq_num - expected_seq_num < MAX_PACKETS) {
            // Buffer the packet if it is within our expected range
            Packet *pkt = &packets[received_seq_num % MAX_PACKETS];
            memcpy(pkt->data, buffer + sizeof(int), bytesRead - sizeof(int));
            pkt->seq_num = received_seq_num;
            pkt->length = bytesRead - sizeof(int);
        }
    }

    fclose(file);
    close(udp_sockfd);
    printf("Música baixada com sucesso.\n");
}

//Caminho inverso para envio de músicas para o servidor
void send_file_udp(const char* filename, struct sockaddr_in cliaddr) {
    int udp_sockfd;
    char buffer[MAXDATASIZE];
    FILE *file;
    struct stat file_stat;
    off_t total_sent = 0;
    double progress = 0.0;
    int sequence_number = 0;  // Sequence number initialization

    // Create UDP socket
    if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Criação do socket UDP falhou.");
        return;
    }

    // Open the file
    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Abrir o arquivo falhou.");
        close(udp_sockfd);
        return;
    }

    // Get file size
    if (stat(filename, &file_stat) != 0) {
        perror("Pegar o tamanho do arquivo falhou.");
        fclose(file);
        close(udp_sockfd);
        return;
    }

    long long file_size = file_stat.st_size;
    int bytesRead;

    // Adding sequence number to each packet
    while ((bytesRead = fread(buffer + sizeof(int), 1, sizeof(buffer) - sizeof(int), file)) > 0) {
        memcpy(buffer, &sequence_number, sizeof(int));  // Prepend sequence number
        sendto(udp_sockfd, buffer, bytesRead + sizeof(int), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
        sequence_number++;  // Increment sequence number
        total_sent += bytesRead;
        progress = (double)total_sent / file_size * 100.0;
        printf("Enviado %ld de %lld bytes (%.2f%%), Seq Num: %d\n", total_sent, file_size, progress, sequence_number);
    }

    sleep(2);
    // Send EOF message
    sendto(udp_sockfd, EOF_MESSAGE, strlen(EOF_MESSAGE) + 1, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
    fclose(file);
    close(udp_sockfd);
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

        printf("aaa%s", recvline);  

        if (strstr(recvline, "Download iniciado para") != NULL) {
            printf("aqui");
            char local_filename[256];
            snprintf(local_filename, sizeof(local_filename), "client/data/%s.mp3", strstr(recvline, "Download iniciado para") + strlen("Download iniciado para") + 1);
            printf("aqui2");
            receive_udp_file(local_filename);
            printf("passouu");
            continue;  
        }

        if (strstr(recvline, "Envie a música") != NULL) {
            char filename[256];

            struct sockaddr_in cliaddr;
            memset(&cliaddr, 0, sizeof(cliaddr));
            cliaddr.sin_family = AF_INET;
            cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Assuming client is on localhost for simplicity
            cliaddr.sin_port = htons(UDP_PORT);

            snprintf(filename, sizeof(filename), "client/data/%s.mp3", strstr(recvline, "Envie a música") + strlen("Envie a música") + 1);
            printf("aaa%s", filename);
            send_file_udp(filename, cliaddr);
            printf("passouu");
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