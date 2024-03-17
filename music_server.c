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
} Musica;

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
    // Envia uma confirmação para o cliente
    char confirmacao[] = "Música cadastrada com sucesso.\n";
    write(sockfd, confirmacao, sizeof(confirmacao));
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
        snprintf(confirmacao, sizeof(confirmacao), "Música %s removida com sucesso.\n", nome_musica);
        write(sockfd, confirmacao, strlen(confirmacao));
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

            // Monta a mensagem com as informações da música
            char resposta[MAXDATASIZE];
            snprintf(resposta, sizeof(resposta), "ID: %d, Título: %s, Artista: %s, Ano: %d, Idioma: %s, Gênero: %s\n",
                   musicas[i].id, musicas[i].titulo, musicas[i].interprete, musicas[i].ano,
                   musicas[i].idioma, musicas[i].genero);

            // Envia a resposta para o cliente
            write(sockfd, resposta, strlen(resposta));

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
        offset += snprintf(resposta + offset, sizeof(resposta) - offset, "ID: %d, Título: %s, Artista: %s, Ano: %d, Idioma: %s, Gênero: %s\n",
                   musicas[i].id, musicas[i].titulo, musicas[i].interprete, musicas[i].ano,
                   musicas[i].idioma, musicas[i].genero);
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

int main (int argc, char **argv) {
    int    listenfd, connfd;
    struct sockaddr_in servaddr;
    char   buf[MAXDATASIZE];
    //time_t ticks;

    struct Musica musicas[MAX_MUSICAS];
    int num_musicas = 0;
    char menu[] = "\n\n===MUSIC SERVER===\n 1. Cadastre uma nova música\n 2. Delete uma música\n 3. Liste as musicas por ano\n 4. Liste as músicas por idioma e ano\n 5. Liste as músicas por gênero\n 6. Liste todas as músicas\n 7. Veja os dados de uma música\n 8. Veja os dados de todas as músicas\n 9. Baixe uma música (Próxima versão)\n q. Fechar\n\nEscolha uma opção:\n";

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(0);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    // Obtendo a porta dinâmica
    struct sockaddr_in dynamicAddr;
    socklen_t addrLen = sizeof(dynamicAddr);
    getsockname(listenfd, (struct sockaddr*)&dynamicAddr, &addrLen);
    printf("Servidor escutando na porta: %d\n", ntohs(dynamicAddr.sin_port));


    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen");
        exit(1);
    }

    for ( ; ; ) {
      if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {
        perror("accept");
        exit(1);
        }

        struct sockaddr_in remoteAddr;
        socklen_t addrLen = sizeof(remoteAddr);
        getpeername(connfd, (struct sockaddr*)&remoteAddr, &addrLen);
        printf("Remote IP: %s\nRemote Port: %d\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));

        // Enviar menu inicial de opções ao cliente
        send_menu(connfd, &menu[0]);

        // Receber escolha do cliente
        while (read(connfd, buf, sizeof(buf)) > 0) {
            handle_client_choice(connfd, buf[0], musicas, &num_musicas);
            sleep(1);
            send_menu(connfd, &menu[0]);  // Envia o menu novamente após lidar com a escolha do cliente
        }

        // Se o cliente fechar a conexão, sai do loop
        if (errno == ECONNRESET) {
            close(connfd);
            break;
        }
    }
    return(0);
}
