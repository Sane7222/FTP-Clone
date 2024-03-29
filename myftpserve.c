/*
 *    Matias Moseley     11/17/2022     CS 360
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<netdb.h>

#define PORT_NUM "49999"
#define BACKLOG 1
#define BUFFER_SIZE 512

void main(int argc, char *argv[]){

    char host[NI_MAXHOST], buffer[BUFFER_SIZE] = {0}, *exitCond = "exit\0";
    int listenfd, connectfd, errNum, num, log = 0;
    struct addrinfo servAddr = {0}, *this;
    struct sockaddr_storage clientAddr;
    socklen_t addrlen = sizeof(struct sockaddr_storage); pid_t here;

    servAddr.ai_family = AF_INET;
    servAddr.ai_socktype = SOCK_STREAM;

    if((errNum = getaddrinfo(NULL, PORT_NUM, &servAddr, &this)) != 0){ // Get internet addr
        fprintf(stderr, "Error: %s\n", gai_strerror(errNum));
        freeaddrinfo(this);
        exit(1);
    }

    // Make a reuseable socket, bind it to a port, and establish as a server 
    if((listenfd = socket(this->ai_family, this->ai_socktype, 0)) == -1 || setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1 || bind(listenfd, this->ai_addr, this->ai_addrlen) == -1 || listen(listenfd, BACKLOG) == -1){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        freeaddrinfo(this);
        exit(1);
    }

    freeaddrinfo(this);

    for(;;){
        if((connectfd = accept(listenfd, (struct sockaddr *) &clientAddr, &addrlen)) == -1){ // Accept connection
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(1);
        }

        log++;
        if((here = fork()) == -1){ // Fork
            fprintf(stderr, "Error: %s\n", strerror(errno));
            close(connectfd);
            exit(1);
        }

        else if(here){ // Parent
            close(connectfd);

            if(waitpid(-1, NULL, WNOHANG) == -1){ // Clean up child
                fprintf(stderr, "Error: %s\n", strerror(errno));
                exit(1);
            }
        }

        else{ // Child
            close(listenfd);

            if((errNum = getnameinfo((struct sockaddr *) &clientAddr, addrlen, host, NI_MAXHOST, NULL, 0, NI_NUMERICSERV)) != 0){ // Convert socket addr to host name
                fprintf(stderr, "Error: %s\n", gai_strerror(errNum));
                exit(1);
            }

            fprintf(stdout, "%s %d\n", host, log);

            for(;;){
                num = read(connectfd, buffer, BUFFER_SIZE); // Write to client and close connection
                fprintf(stdout, "%s\n", buffer);
                if(!strncmp(buffer, exitCond, num)){  // Exit recieved
                    fprintf(stdout, "Server Child Process Exiting\n");
                    exit(0);
                }
            }
        }
    }
}
