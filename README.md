# MC833 - Programação em Redes de Computadores

Este é um servidor de música simples que permite o cadastro, remoção e listagem de músicas. O servidor foi implementado em C usando sockets TCP e multithreading para lidar com múltiplos clientes simultaneamente.

## Requisitos

- Compilador GCC (para compilar o código em C)
- Conexão de rede para comunicação com clientes

## Compilação

### Iniciando o servidor:

gcc server/music_server.c -Wall -o servidor -pthread

./servidor

### Iniciando o cliente:

gcc client/music_client.c -Wall -o cliente

./cliente 127.0.0.1 <insira a mesma porta que o servidor informa que está ouvindo>

## Notas

- Certifique-se de que o diretório `server/data` exista e tenha permissões de leitura e escrita para o usuário que está executando o servidor. Esse diretório é usado para armazenar os arquivos de música.
