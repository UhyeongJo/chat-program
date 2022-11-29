#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>

#ifdef C4
    #define SERVER_PATH "c4"
#endif

#define BUFFER_LENGTH 1024

void main(){
    int sd_un;
    int flags_un;
    int strlength;

    char buf[BUFFER_LENGTH];
    char *quit = "quit\n";

    struct sockaddr_un ser_un;

    memset((char *)&ser_un, '\0', sizeof(ser_un));

    if((sd_un = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(1);
    }
    flags_un = fcntl(sd_un, F_GETFL, 0);
    fcntl(sd_un, F_SETFL, flags_un | O_NONBLOCK);

    ser_un.sun_family = AF_UNIX;
    strcpy(ser_un.sun_path, SERVER_PATH);

    if(connect(sd_un, (struct sockaddr *)&ser_un, sizeof(ser_un)) == -1){
        perror("connect");
        exit(1);
    }
    else{
        printf("[Info] Unix socket : connected to the server\n");
    }

    while(1){
        printf("> Enter : ");
        fgets(buf, BUFFER_LENGTH, stdin);

        if(send(sd_un, buf, strlen(buf), 0) == -1){
            perror("send");
            exit(1);
        }
        if(strcmp(buf, "3\n") == 0){
            exit(1);
        }
        
    }
    printf("[Info] Closing socket");
    close(sd_un);
}