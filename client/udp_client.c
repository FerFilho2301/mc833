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

    int sockfd, bytesRead;  // Socket para o servidor e para o cliente.
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

    while (1)
    {
        printf("\nPlease enter file name to receive:\n"); 
        scanf("%s", buffer); 


        sprintf(filePath, "client/data/%s", buffer); // Constructing file path
        file = fopen(filePath, "w");

        if (file == NULL) {
            printf("Download failed!\n");
            break;
        }

        sendto(sockfd, buffer, sizeof(buffer), sendrecvflag, (struct sockaddr*)&servaddr, servaddrLen);

        // receive  
        while ((bytesRead = recvfrom(sockfd, buffer, sizeof(buffer), sendrecvflag, (struct sockaddr*)&servaddr, &servaddrLen)) > 0 && strcmp(buffer, EOF_MESSAGE) != 0)
        {
            fwrite(buffer, 1, bytesRead, file);
            printf("\nDownloading File...\n");
        }

        if (file != NULL) 
            fclose(file);
            
        printf("\nDownload complete\n");
    }
    
}