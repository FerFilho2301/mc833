#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080 
#define MAXDATASIZE 64
#define sendrecvflag 0 
#define nofile "File Not Found!" 
#define cipherKey 'S' 

void clearBuffer(char* b) {
    int i; 
    for (i = 0; i < MAXDATASIZE; i++) 
        b[i] = '\0'; 
} 
  
// function to encrypt 
char Cipher(char ch) { 
    return ch ^ cipherKey; 
} 

int recvFile(FILE* file, char* buf, int s) 
{ 
    int i; 
    char ch; 
    for (i = 0; i < s; i++) { 
        ch = buf[i]; 
        ch = Cipher(ch); 
        if (ch == EOF) 
            return 1; 
        fwrite(&ch, sizeof(char), 1, file); // Write encrypted character to file
    }
    return 0; 
}  

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

        sendto(sockfd, buffer, MAXDATASIZE, 
               sendrecvflag, (struct sockaddr*)&servaddr, 
               servaddrLen);

        while (1) { 
            // receive 
            clearBuffer(buffer); 
            nBytes = recvfrom(sockfd, buffer, MAXDATASIZE, 
                              sendrecvflag, (struct sockaddr*)&servaddr, 
                              &servaddrLen); 
  
            // process 
            if (recvFile(file, buffer, MAXDATASIZE)) { 
                break; 
            } 
        } 

        fclose(file);
        printf("\nDownload complete\n");
    }
    
}