#include "music_server.h"

Musica musicas[MAX_MUSICAS];
int num_musicas = 0;

#define UDP_PORT 8080

//Caminho inverso para envio de músicas
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

void handle_download(Musica *musicas, int num_musicas, int sockfd) {
    int id;
    char response[MAXDATASIZE];

    write(sockfd, "Informe o ID da música a ser baixada: ", strlen("Informe o ID da música a ser baixada: "));
    sleep(1);
    write(sockfd, CLI_INT, strlen(CLI_INT));
    read(sockfd, &id, sizeof(id));

    for (int i = 0; i < num_musicas; i++) {
        if (musicas[i].id == id) {
            snprintf(response, sizeof(response), "Download iniciado para %s\n", musicas[i].titulo);
            write(sockfd, response, strlen(response));
            
            struct sockaddr_in cliaddr;
            memset(&cliaddr, 0, sizeof(cliaddr));
            cliaddr.sin_family = AF_INET;
            cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Assuming client is on localhost for simplicity
            cliaddr.sin_port = htons(UDP_PORT);

            char filename[256];
            snprintf(filename, sizeof(filename), "server/data/%s.mp3", musicas[i].titulo);
            sleep(3);
            send_file_udp(filename, cliaddr);
            printf("sent file");
            
            return;
        }
    }
    snprintf(response, sizeof(response), "Música não encontrada.\n");
    write(sockfd, response, strlen(response));
}

void carregar_musicas(Musica *musicas, int *num_musicas) {
    DIR *dirpath;
    struct dirent *entry;
    FILE *arquivo;
    char nomeArquivo[MAXDATASIZE];
    struct stat file_stat;
    int loaded = 0; // Variável para controlar se alguma música foi carregada

    
    // Abre o diretório "data"
    if ((dirpath = opendir("server/data")) != NULL) {
                
        // Itera sobre todos os arquivos no diretório
        while ((entry = readdir(dirpath)) != NULL) {
            if (strstr(entry->d_name, ".txt") != NULL){
                sprintf(nomeArquivo, "server/data/%s", entry->d_name);

                if (stat(nomeArquivo, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {

                    arquivo = fopen(nomeArquivo, "r");
                    if (arquivo != NULL) {
                        // Lê as informações da música do arquivo
                        Musica nova_musica;
                        fscanf(arquivo, "ID: %d\n", &nova_musica.id);
                        fscanf(arquivo, "Título: %[^\n]\n", nova_musica.titulo);
                        fscanf(arquivo, "Intérprete: %[^\n]\n", nova_musica.interprete);
                        fscanf(arquivo, "Idioma: %[^\n]\n", nova_musica.idioma);
                        fscanf(arquivo, "Gênero: %[^\n]\n", nova_musica.genero);
                        fscanf(arquivo, "Ano: %d\n", &nova_musica.ano);
                        fscanf(arquivo, "Refrão: %[^\n]\n", nova_musica.refrao);

                        // Adiciona a música ao array de músicas
                        musicas[*num_musicas] = nova_musica;
                        (*num_musicas)++;

                        loaded++;

                        // Fecha o arquivo
                        fclose(arquivo);
                    }
                }
            }
        }

        // Fecha o diretório
        closedir(dirpath);

        printf("Músicas carregadas do banco de dados: %d\n", loaded);        
    } else {
        // Se houver erro ao abrir o diretório, exibe uma mensagem de erro
        perror("Erro ao abrir o diretório 'data'");
    }
}

// Opções selecionáveis pelo menu:
void cadastrar_musica(Musica *musicas, int *num_musicas, int sockfd) {
    Musica nova_musica;

    // Recebe os dados da nova música do cliente
    write(sockfd, "Informe o título da música: ", strlen("Informe o título da música: "));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
    read(sockfd, nova_musica.titulo, sizeof(nova_musica.titulo));
    
    write(sockfd, "Informe o nome do artista: ", strlen("Informe o nome do artista: "));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
    read(sockfd, nova_musica.interprete, sizeof(nova_musica.interprete));

    write(sockfd, "Informe o idioma da música: ", strlen("Informe o idioma da música: "));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
    read(sockfd, nova_musica.idioma, sizeof(nova_musica.idioma));

    write(sockfd, "Informe o gênero da música: ", strlen("Informe o gênero da música: "));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
    read(sockfd, nova_musica.genero, sizeof(nova_musica.genero));

    write(sockfd, "Informe o ano de lançamento: ", strlen("Informe o ano de lançamento: "));
    sleep(1);
    write(sockfd, CLI_INT, strlen(CLI_INT));
    read(sockfd, &nova_musica.ano, sizeof(nova_musica.ano));

    write(sockfd, "Informe o refrão da música: ", strlen("Informe o refrão da música: "));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
    read(sockfd, nova_musica.refrao, sizeof(nova_musica.refrao));

    // Verifica se há espaço disponível para mais músicas
    if (*num_musicas >= MAX_MUSICAS) {
        printf("Não é possível cadastrar mais músicas. Limite alcançado.\n");
        return;
    }

    // Atribui um identificador único para a música
    nova_musica.id = *num_musicas + 1;

    // Adiciona a nova música ao array de músicas
    musicas[*num_musicas] = nova_musica;

    // Incrementa o contador de músicas
    (*num_musicas)++;

    printf("Música cadastrada com sucesso.\n");

    // Escreve as informações da música em um arquivo de texto
    // Cria um nome de arquivo baseado no ID da música
    char nomeArquivo[100];
    sprintf(nomeArquivo, "server/data/%d.txt", nova_musica.id);
    
    FILE *arquivo = fopen(nomeArquivo, "w");

    // Verifica se o arquivo foi aberto com sucesso
    if (arquivo == NULL) {
        printf("Erro ao armazenar música em arquivo.\n");
        return;
    }else
    {
        // Escreve os dados da música no arquivo
        fprintf(arquivo, "ID: %d\n", nova_musica.id);
        fprintf(arquivo, "Título: %s\n", nova_musica.titulo);
        fprintf(arquivo, "Intérprete: %s\n", nova_musica.interprete);
        fprintf(arquivo, "Idioma: %s\n", nova_musica.idioma);
        fprintf(arquivo, "Gênero: %s\n", nova_musica.genero);
        fprintf(arquivo, "Ano: %d\n", nova_musica.ano);
        fprintf(arquivo, "Refrão: %s\n", nova_musica.refrao);
        
        fclose(arquivo);

        printf("Música salva em: %s\n",nomeArquivo);
    }

    write(sockfd, "Música cadastrada com sucesso! Deseja incluir um .mp3? (s/n): ", strlen("Música cadastrada com sucesso! Deseja incluir um .mp3? (s/n): "));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
    sleep(1);
    char incluirMP3;
    read(sockfd, &incluirMP3, sizeof(incluirMP3));
    sleep(1);
    if (incluirMP3 == 's' || incluirMP3 == 'S') {
        char mp3Filename[256];
        sprintf(mp3Filename, "server/data/%s.mp3", nova_musica.titulo);
        receive_udp_file(mp3Filename);
        char message[512];
        snprintf(message, sizeof(message), "Envie a música %s", nova_musica.titulo);
        write(sockfd, message, strlen(message));
    }
    sleep(1);
}

void remover_musica(Musica *musicas, int *num_musicas, int sockfd) {
    int encontrado = 0;
    int id;
    char nome_musica[100];

    // Recebe o id da música
    write(sockfd, "Informe o ID da música a ser deletada: ", strlen("Informe o ID da música a ser deletada: "));
    sleep(1);
    write(sockfd, CLI_INT, strlen(CLI_INT));
    read(sockfd, &id, sizeof(id));

    // Procura a música com o identificador informado
    for (int i = 0; i < *num_musicas; i++) {
        if (musicas[i].id == id) {
            encontrado = 1;
            strcpy(nome_musica, musicas[i].titulo);

            // Move as músicas à frente uma posição para trás para preencher o espaço
            for (int j = i; j < *num_musicas - 1; j++) {
                musicas[j] = musicas[j + 1];
            }
            (*num_musicas)--;
            break;
        }
    }// Se houver apenas uma música restante, esvazia o vetor
    if (*num_musicas == 0) {
        musicas[0] = (Musica){0}; // Atribui valores nulos ou vazios
    }

    // Se a música foi encontrada, envia uma confirmação para o cliente
    if (encontrado) {
        char confirmacao[150];
        snprintf(confirmacao, sizeof(confirmacao), "Música '%s' removida com sucesso.\n", nome_musica);
        write(sockfd, confirmacao, strlen(confirmacao));

        // Remove o arquivo de texto associado à música
        char nomeArquivo[100];
        sprintf(nomeArquivo, "server/data/%d.txt", id);
        if (unlink(nomeArquivo) == -1) {
            perror("Erro ao excluir o arquivo de música");
        }

        printf("Música server/data/%d.txt removida com sucesso.\n", id);

    } else {
        char erro[] = "Música com o identificador informado não encontrada.\n";
        write(sockfd, erro, sizeof(erro));
    }
}

void listar_por_ano(Musica *musicas, int num_musicas, int sockfd) {
    char resposta[MAXDATASIZE];
    int offset = 0;
    int ano;

    // Recebe o ano da música
    write(sockfd, "Informe o ano: ", strlen("Informe o ano: "));
    sleep(1);
    write(sockfd, CLI_INT, strlen(CLI_INT));
    read(sockfd, &ano, sizeof(ano));

    // Monta a mensagem com as músicas lançadas no ano especificado
    for (int i = 0; i < num_musicas; i++) {
        if (musicas[i].ano == ano) {
            offset += snprintf(resposta + offset, sizeof(resposta) - offset, "ID: %d, Título: %s, Artista: %s\n", musicas[i].id, musicas[i].titulo, musicas[i].interprete);
        }
    }
    if (offset == 0){
        write(sockfd, "Não há músicas para listar.", strlen("Não há músicas para listar."));
    }else{
        // Envia a resposta para o cliente
        write(sockfd, resposta, strlen(resposta));
    }
}

void listar_por_idioma_ano(Musica *musicas, int num_musicas, int sockfd) {
    char resposta[MAXDATASIZE];
    int offset = 0;
    Musica nova_musica;

    // Recebe os dados da música do cliente
    write(sockfd, "Informe o idioma da música: ", strlen("Informe o idioma da música: "));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
    read(sockfd, nova_musica.idioma, sizeof(nova_musica.idioma));

    write(sockfd, "Informe o ano de lançamento: ", strlen("Informe o ano de lançamento: "));
    sleep(1);
    write(sockfd, CLI_INT, strlen(CLI_INT));
    read(sockfd, &nova_musica.ano, sizeof(nova_musica.ano));

    // Monta a mensagem com as músicas lançadas no idioma e ano especificados
    for (int i = 0; i < num_musicas; i++) {
        if (strcmp(musicas[i].idioma, nova_musica.idioma) == 0 && musicas[i].ano == nova_musica.ano) {
            offset += snprintf(resposta + offset, sizeof(resposta) - offset, "ID: %d, Título: %s, Artista: %s\n", musicas[i].id, musicas[i].titulo, musicas[i].interprete);
        }
    }
    if (offset == 0){
        write(sockfd, "Não há músicas para listar.", strlen("Não há músicas para listar."));
    }else{
        // Envia a resposta para o cliente
        write(sockfd, resposta, strlen(resposta));
    }
}

void listar_por_genero(Musica *musicas, int num_musicas, int sockfd) {
    char resposta[MAXDATASIZE];
    int offset = 0;
    char genero[30];

    // Recebe o gênero
    write(sockfd, "Informe o gênero da música: ", strlen("Informe o gênero da música: "));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
 
    if (read(sockfd, &genero, sizeof(genero)) != sizeof(genero)) {
        perror("Erro ao receber id para exclusão");
        return;
    }

    // Monta a mensagem com as músicas do gênero especificado
    for (int i = 0; i < num_musicas; i++) {
        if (strcmp(musicas[i].genero, genero) == 0) {
            offset += snprintf(resposta + offset, sizeof(resposta) - offset, "ID: %d, Título: %s, Artista: %s\n", musicas[i].id, musicas[i].titulo, musicas[i].interprete);
        }
    }

    if (offset == 0){
        write(sockfd, "Não há músicas para listar.", strlen("Não há músicas para listar."));
    }else{
        // Envie a resposta para o cliente
        write(sockfd, resposta, strlen(resposta));
    }
}

void listar_todas(Musica *musicas, int num_musicas, int sockfd) {
    char resposta[MAXDATASIZE];
    int offset = 0;

    // Monta a mensagem com todas as músicas cadastradas
    for (int i = 0; i < num_musicas; i++) {
        offset += snprintf(resposta + offset, sizeof(resposta) - offset, "ID: %d, Título: %s, Artista: %s\n", musicas[i].id, musicas[i].titulo, musicas[i].interprete);
    }

    if (offset == 0){
        write(sockfd, "Não há músicas para listar.", strlen("Não há músicas para listar."));
    }else{
        // Envie a resposta para o cliente
        write(sockfd, resposta, strlen(resposta));
    }
}

void listar_info_por_id(Musica *musicas, int num_musicas, int sockfd) {
    int encontrado = 0;
    int id;

    // Recebe o ID da música
    write(sockfd, "Informe o ID:", 14);
    sleep(1);
    write(sockfd, CLI_INT, strlen(CLI_INT));
    if (read(sockfd, &id, sizeof(id)) != sizeof(id)) {
        perror("Erro ao receber id para exclusão");
        return;
    }

    // Procura a música com o identificador informado
    for (int i = 0; i < num_musicas; i++) {
        if (musicas[i].id == id) {
            encontrado = 1;

            // Calcula o tamanho necessário para a string formatada
            int tamanho = snprintf(NULL, 0, "ID: %d, Título: %s, Artista: %s, Ano: %d, Idioma: %s, Gênero: %s, Refrão: %s\n",
                                    musicas[i].id, musicas[i].titulo, musicas[i].interprete, musicas[i].ano,
                                    musicas[i].idioma, musicas[i].genero, musicas[i].refrao);
            
            // Aloca dinamicamente o buffer com o tamanho calculado
            char *resposta = malloc(tamanho + 1); // +1 para o caractere nulo de terminação

            if (resposta == NULL) {
                perror("Erro ao alocar memória para resposta");
                return;
            }

            // Formata a string no buffer alocado dinamicamente
            snprintf(resposta, tamanho + 1, "ID: %d, Título: %s, Artista: %s, Ano: %d, Idioma: %s, Gênero: %s, Refrão: %s\n",
                     musicas[i].id, musicas[i].titulo, musicas[i].interprete, musicas[i].ano,
                     musicas[i].idioma, musicas[i].genero, musicas[i].refrao);

            // Envia a resposta para o cliente
            write(sockfd, resposta, strlen(resposta));

            // Libera a memória alocada dinamicamente
            free(resposta);

            break;
        }
    }

    // Se a música não for encontrada, envie uma mensagem de erro para o cliente
    if (!encontrado) {
        char erro[] = "Música com o identificador informado não encontrada.\n";
        write(sockfd, erro, sizeof(erro));
    }
}

void listar_todas_info(Musica *musicas, int num_musicas, int sockfd) {
    char resposta[MAXDATASIZE];
    int offset = 0;

    // Monta a mensagem com todas as músicas cadastradas
    for (int i = 0; i < num_musicas; i++) {
        offset += snprintf(resposta + offset, sizeof(resposta) - offset, "ID: %d, Título: %s, Artista: %s, Ano: %d, Idioma: %s, Gênero: %s, Refrão: %s\n",
                   musicas[i].id, musicas[i].titulo, musicas[i].interprete, musicas[i].ano,
                   musicas[i].idioma, musicas[i].genero, musicas[i].refrao);
    }

    if (offset == 0){
        write(sockfd, "Não há músicas para listar.", strlen("Não há músicas para listar."));
    }else{
        // Envia a resposta para o cliente
        write(sockfd, resposta, strlen(resposta));
    }
}

// Função para enviar o menu de opções ao cliente e solicitar resposta
void send_menu(int sockfd, char *menu) {
    write(sockfd, menu, strlen(menu));
    sleep(1);
    write(sockfd, CLI_STRING, strlen(CLI_STRING));
    sleep(1);
}

// Função para lidar com a escolha do cliente no menu
void handle_client_choice(int sockfd, char choice, Musica *musicas, int *num_musicas) {
    char response[MAXDATASIZE];
    sleep(1);
    switch (choice) {
        case '1':
            snprintf(response, sizeof(response), "Você escolheu a opção 1.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            cadastrar_musica(musicas, num_musicas, sockfd);
            break;
        case '2':
            snprintf(response, sizeof(response), "Você escolheu a opção 2.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            remover_musica(musicas, num_musicas, sockfd);
            break;
        case '3':
            snprintf(response, sizeof(response), "Você escolheu a opção 3.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            listar_por_ano(musicas, *num_musicas, sockfd);
            break;
        case '4':
            snprintf(response, sizeof(response), "Você escolheu a opção 4.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            listar_por_idioma_ano(musicas, *num_musicas, sockfd);
            break;
        case '5':
            snprintf(response, sizeof(response), "Você escolheu a opção 5.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            listar_por_genero(musicas, *num_musicas, sockfd);
            break;
        case '6':
            snprintf(response, sizeof(response), "Você escolheu a opção 6.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            listar_todas(musicas, *num_musicas, sockfd);
            break;
        case '7':
            snprintf(response, sizeof(response), "Você escolheu a opção 7.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            listar_info_por_id(musicas, *num_musicas, sockfd);
            break;
        case '8':
            snprintf(response, sizeof(response), "Você escolheu a opção 8.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            listar_todas_info(musicas, *num_musicas, sockfd);
            break;
        case '9':
            handle_download(musicas, *num_musicas, sockfd);
            break;
        case 'q':
            snprintf(response, sizeof(response), "Você escolheu a opção quit.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            close(sockfd);
            break;
        default:
            snprintf(response, sizeof(response), "Opção inválida.\n");
            write(sockfd, response, strlen(response));
            sleep(1);
            break;
    }sleep(1);
}

// Função executada por cada thread para cada cliente
void *client_handler(void *socket_desc) {
    // Extrai o socket
    int connfd = *(int*)socket_desc;
    free(socket_desc);
    char buf[MAXDATASIZE];  // Buffer para armazenar os dados recebidos do cliente
    // Menu que será enviado ao cliente
    char menu[] = "\n\n===MUSIC SERVER===\n 1. Cadastre uma nova música\n 2. Delete uma música\n 3. Liste as musicas por ano\n 4. Liste as músicas por idioma e ano\n 5. Liste as músicas por gênero\n 6. Liste todas as músicas\n 7. Veja os dados de uma música\n 8. Veja os dados de todas as músicas\n 9. Baixe uma música\n q. Fechar\n\nEscolha uma opção:\n";

    // Envia o menu ao cliente
    send_menu(connfd, menu);

    // Lê os dados do cliente enquanto houver dados a serem lidos
    while (read(connfd, buf, sizeof(buf)) > 0) {
        // Processa a escolha do clinte
        handle_client_choice(connfd, buf[0], musicas, &num_musicas);
        // Reenvia o menu ao cliente
        send_menu(connfd, menu);
    }

    // Quando termina a interação, o socket é fechado
    close(connfd);
    return NULL;
}

int main(void) {
    int listenfd, connfd;  // Socket para o servidor e para o cliente.
    struct sockaddr_in servaddr;
    pthread_t thread_id;  // ID de thread para as threads de tratamento de cliente

    // Carrega as músicas do diretório "data"
    carregar_musicas(musicas, &num_musicas);

    // Socket para o servidor
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Configura o endereço do servidor para aceitar conexões em qualquer endereço IP do servidor
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(0);

    // Associa o socket à porta e ao endereço especificados acima
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    // Inicia a escuta de conexões no socket
    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen");
        exit(1);
    }

    // Recupera e exibe a porta na qual o servidor está ouvindo
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(listenfd, (struct sockaddr *)&sin, &len) == -1) {
        perror("getsockname");
    } else {
        printf("Listening on port %d\n", ntohs(sin.sin_port));
    }

    // Loop principal para aceitar conexões de clientes
    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1) {
            perror("accept");
            continue;  // Continua o loop se houver erro ao aceitar a conexão
        }

        // Aloca memória para o socket do cliente
        int *new_sock = malloc(sizeof(int));
        *new_sock = connfd;

        // Cria uma thread para cada cliente conectado
        if (pthread_create(&thread_id, NULL, client_handler, (void*) new_sock) != 0) {
            perror("pthread_create");
        }
        pthread_detach(thread_id);  // Permite que a thread seja liberada automaticamente ao terminar
    }

    return 0;
}
