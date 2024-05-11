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
  
// function sending file 
int sendFile(FILE* fp, char* buf, int s) { 
        int i, len; 
        if (fp == NULL) { 
                strcpy(buf, nofile); 
                len = strlen(nofile); 
                buf[len] = EOF; 
                for (i = 0; i <= len; i++) 
                        buf[i] = Cipher(buf[i]); 
                return 1; 
        } 
  
        char ch, ch2; 
        for (i = 0; i < s; i++) { 
                ch = fgetc(fp); 
                ch2 = Cipher(ch); 
                buf[i] = ch2; 
                if (ch == EOF) 
                        return 1; 
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
        
                clearBuffer(buffer);

                nBytes = recvfrom(sockfd, buffer, 
                                MAXDATASIZE, sendrecvflag, 
                                (struct sockaddr*)&servaddr, &servaddrLen);

                printf("\nFile Name Received: %s\n", buffer); 

                sprintf(filePath, "server/data/%s", buffer); // Constructing file path
                file = fopen(filePath, "r"); 

                if (file == NULL) 
                        printf("\nFile open failed!\n"); 
                else
                        printf("\nFile Successfully opened!\n");

                while (1)
                {
                        // process 
                        if (sendFile(file, buffer, MAXDATASIZE)) { 
                                sendto(sockfd, buffer, MAXDATASIZE, 
                                sendrecvflag,  
                                (struct sockaddr*)&servaddr, servaddrLen); 
                                break; 
                        } 
                
                        // send 
                        sendto(sockfd, buffer, MAXDATASIZE, 
                                sendrecvflag, 
                                (struct sockaddr*)&servaddr, servaddrLen); 
                        clearBuffer(buffer); 
                }

                if (file != NULL) 
                        fclose(file);
        }

   return 0;
}