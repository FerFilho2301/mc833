# MC833 - Programação em Redes de Computadores

### Como iniciar o servidor:

gcc music_server.c -Wall -o servidor 

./servidor

### Instanciar um cliente:

gcc music_client.c -Wall -o cliente 

./cliente 127.0.0.1 <mesmo port que o servidor informa que está ouvindo>
