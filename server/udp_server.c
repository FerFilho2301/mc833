#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080 
#define MAXDATASIZE 2048
#define sendrecvflag 0 
#define EOF_MESSAGE "EOF"

int main() {

        int sockfd, nBytes;  // Socket para o servidor e para o cliente.
        struct sockaddr_in servaddr;
        int servaddrLen = sizeof(servaddr); 
        FILE* file;

        char buffer[MAXDATASIZE], filePath[MAXDATASIZE + 13];

        //Create a UDP Socket
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("Socket Creation Failed");
                exit(1);
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(PORT);

        //Bind server address to socket descriptor
        if (bind(sockfd, (const struct sockaddr *)&servaddr, servaddrLen) < 0) {
                perror("Binding Failed!");
                exit(1);
        }
        else{
                printf("Socket Successfully binded!\n");
        }
        
        while (1)
        {
                printf("Waiting for file name...\n"); 
        
                nBytes = recvfrom(sockfd, buffer, 
                                sizeof(buffer), sendrecvflag, 
                                (struct sockaddr*)&servaddr, &servaddrLen);

                printf("\nFile Name Received: %s\n", buffer); 

                sprintf(filePath, "server/data/%s", buffer); // Constructing file path
                file = fopen(filePath, "rb"); 

                if (file == NULL) 
                        printf("\nFile open failed!\n"); 
                else
                        printf("\nFile Successfully opened!\n");

                
                int bytesRead;
                while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
                {
                        sendto(sockfd, buffer, bytesRead, 0, (struct sockaddr*)&servaddr, servaddrLen);
                        printf("\nSending File...\n");
                }

                // Send EOF message
                sendto(sockfd, EOF_MESSAGE, strlen(EOF_MESSAGE) + 1, 0, (struct sockaddr*)&servaddr, servaddrLen);
                if (file != NULL) 
                        fclose(file);

                printf("\nDownload Complete...\n");
        }

   return 0;
}