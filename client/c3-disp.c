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

#ifdef C3
    #define SERVER_PATH "c3"
#endif

#define PORTNUM 9000    // port number
#define BUFFER_LENGTH 1024

void main(){
    int s_un;
    int sd_un, sd_in;
    int clen_un;
    int flags_un, flags_in;
    int un_length, in_length;

    char *quit = "quit\n";
    char me_buf[BUFFER_LENGTH] = {0,};
    char you_buf[BUFFER_LENGTH] = {0,};
    char none_buf[BUFFER_LENGTH] = {0,};    //send

    struct sockaddr_un ser_un, cli_un;
    struct sockaddr_in ser_in;

    /////////////////////////// UNIX SERVER //////////////////////////////
    if((s_un = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(1);
    }

    memset((char *)&ser_un, '\0', sizeof(ser_un));

    ser_un.sun_family = AF_UNIX;
    strcpy(ser_un.sun_path, SERVER_PATH);

    if(bind(s_un, (struct sockaddr *)&ser_un, sizeof(ser_un)) == -1){
        perror("bind");
        exit(1);
    }

    if(listen(s_un, 5) < 0){
        perror("listen");
        exit(1);
    }
    else{
        printf("[Info] Unix socket : waiting for conn req\n");
    }

    clen_un = sizeof(cli_un);

    if((sd_un = accept(s_un, (struct sockaddr *)&cli_un, &clen_un))== -1){
        perror("accept");
        exit(1);
    }
    else{
        printf("[Info] Unix socket : client connected\n");
    }

    flags_un = fcntl(sd_un, F_GETFL, 0);
    fcntl(sd_un, F_SETFL, flags_un | O_NONBLOCK);
    //////////////////////////////////////////////////////////////////////

    ///////////////////////// INET CLIENT ////////////////////////////////
    if((sd_in = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(1);
    }

    memset((char *)&ser_in, '\0', sizeof(ser_in));

    ser_in.sin_family = AF_INET;
    ser_in.sin_port = htons(PORTNUM);
    ser_in.sin_addr.s_addr = inet_addr("10.0.2.15");

    if(connect(sd_in, (struct sockaddr *)&ser_in, sizeof(ser_in)) == -1){
        perror("connect");
        exit(1);
    }
    else{
        printf("[Info] Inet socket : connected to the server\n");
    }

    flags_in = fcntl(sd_in, F_GETFL, 0);
    fcntl(sd_in, F_SETFL, flags_in | O_NONBLOCK);
    ///////////////////////////////////////////////////////////////////////

    while(1){
        //send & recv [unix]
        un_length = recv(sd_un, me_buf, sizeof(me_buf), 0);

        if(un_length > 0){
            me_buf[un_length] = 0;
            printf("[ME] : %s", me_buf);

            // send to server
            if(send(sd_in, me_buf, strlen(me_buf), 0) == -1){
                perror("send");
                exit(1);
            }
            if(strcmp(me_buf, "3\n") == 0){
                exit(1);
            }
        }
        else{
            memset(me_buf, 0, sizeof(me_buf));
        }
     
        //send & recv [inet]
        in_length = recv(sd_in, you_buf, sizeof(you_buf), 0);

        if(in_length > 0){
            you_buf[in_length] = 0;

            printf("%s", you_buf);

        }
        else{
            memset(you_buf, 0, sizeof(you_buf));
        }
    }
}