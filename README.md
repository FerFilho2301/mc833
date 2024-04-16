# MC833 - Programação em Redes de Computadores

### Como iniciar o servidor:

gcc server/music_server.c -Wall -o servidor -pthread

./servidor

### Instanciar um cliente:

gcc client/music_client.c -Wall -o cliente -pthread

./cliente 127.0.0.1 <mesmo port que o servidor informa que está ouvindo>
