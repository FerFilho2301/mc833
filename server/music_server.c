#include "music_server.h"

Musica musicas[MAX_MUSICAS];
int num_musicas = 0;

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
    
    // Envia uma confirmação para o cliente
    char confirmacao[150];
    sprintf(confirmacao, "Música cadastrada com sucesso! Seu ID = %d\n", nova_musica.id);
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

            // Monta a mensagem com as informações da música
            char resposta[MAXDATASIZE];
            snprintf(resposta, sizeof(resposta), "ID: %d, Título: %s, Artista: %s, Ano: %d, Idioma: %s, Gênero: %s, Refrão: %s\n",
                   musicas[i].id, musicas[i].titulo, musicas[i].interprete, musicas[i].ano,
                   musicas[i].idioma, musicas[i].genero, musicas[i].refrao);

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
    char menu[] = "\n\n===MUSIC SERVER===\n 1. Cadastre uma nova música\n 2. Delete uma música\n 3. Liste as musicas por ano\n 4. Liste as músicas por idioma e ano\n 5. Liste as músicas por gênero\n 6. Liste todas as músicas\n 7. Veja os dados de uma música\n 8. Veja os dados de todas as músicas\n 9. Baixe uma música (Próxima versão)\n q. Fechar\n\nEscolha uma opção:\n";

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
