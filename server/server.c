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
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

#define PORTNUM 9000
#define BUFFER_LENGTH 1024
#define TOTAL_CLIENTS 15
#define ROOM_CLIENTS 5


struct ChatRoom{
    int user_sd[ROOM_CLIENTS];
    int user_cnt;
    //int room_num;
};

struct UserSockets{
    int user_sd;
};

typedef struct ChatInfo{
    struct ChatRoom room;   // chatroom info
    struct UserSockets new_users;      // new user in chatroom
    struct UserSockets returned_users; // exit user in chatroom
}ChatInfo;


//######################## global variables##############################
int s;
int thread_ret;
int thread_length;

fd_set main_readfds, thread_readfds;

int wait_sock[TOTAL_CLIENTS];   // 대기실 소켓
int room_num[3] = {0, 1, 2};

char thread_buf[BUFFER_LENGTH] = {0,};
char spread_buf[BUFFER_LENGTH] = {0,};
char quit_msg[BUFFER_LENGTH] = "quit\n";


pthread_t p_thread[3];
// struct ChatInfo chatinfo[3]; // [i]번 채팅방의 정보를 관리하기 위한 구조체
ChatInfo chatinfo[3]; // [i]번 채팅방의 정보를 관리하기 위한 구조체

struct timeval timeout;     // select timeout

//#########################################################################


int maxArr(int *socket_arr, int n){
    int max = -1;

    for(int i = 0; i < n; i++){
        if(socket_arr[i] > max){
            max = socket_arr[i];
        }
    }
    return max;
}

void handler(int sig){
    printf("\n(시그널 핸들러) 마무리 작업 시작!\n");

    // add 'return resources'
    // socket(close), memory(free), fork(wait), thread(join)..

    for(int i = 0; i < 3; i++){
        pthread_kill(p_thread[i], SIGUSR1);
        pthread_join(p_thread[i], NULL);
    }
    close(s);
    printf("완료! Bye~");
    exit(EXIT_SUCCESS);
}

void thread_exit(int sig)
{
	pthread_exit(NULL);
}

void *thread_func(void *arg);



void main(){
    
    int clen;
    int flags;
    int main_length, main_ret;
    int k;

    char main_buf[BUFFER_LENGTH] = {0,};
    char menu_buf[BUFFER_LENGTH] = "<MENU>\n1.채팅방 목록 보기\n2.채팅방 참여하기(사용법 : 2 <채팅방 번호>)\n3.프로그램 종료\n(0을 입력하면 메뉴가 다시 표시됩니다)\n\n";

    struct sockaddr_in ser, cli;
    
    // 구조체 초기화
    for(int i = 0; i < 3; i++){
        //chatinfo[i].room.room_num = i;
        chatinfo[i].room.user_cnt = 0;
        for(int j = 0; j < ROOM_CLIENTS; j++){ 
            chatinfo[i].room.user_sd[j] = -1;
        }
        chatinfo[i].new_users.user_sd = -1;
        chatinfo[i].returned_users.user_sd = -1;
    }

    // 대기실 소켓 초기화
    for(int i = 0; i < TOTAL_CLIENTS; i++){
        wait_sock[i] = -1;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(1);
    }

    memset((char *)&ser, 0, sizeof(struct sockaddr_in));

    ser.sin_family = AF_INET;
    ser.sin_port = htons(PORTNUM);
    ser.sin_addr.s_addr = inet_addr("10.0.2.15");

    if (bind(s, (struct sockaddr *)&ser, sizeof(ser)) == -1){
        perror("bind");
        exit(1);
    }

    if (listen(s, 5) < 0){
        perror("listen");
        exit(1);
    }

    // client accept
    for(int i = 0; i < 4; i++){
        if ((wait_sock[i] = accept(s, (struct sockaddr *)&cli, &clen)) == -1){
            perror("accept");
            exit(1);
        }
        flags = fcntl(wait_sock[i], F_GETFL, 0); 
        fcntl(wait_sock[i], F_SETFL, flags | O_NONBLOCK); 
    }

    signal(SIGINT, handler);

    for(int i = 0; i < 4; i++){
        if (send(wait_sock[i], menu_buf, strlen(menu_buf), 0) == -1){
            perror("send");
            exit(1);
        }
    }

    // thread create
    for (int i = 0; i < 3; i++){
        if ((pthread_create(&p_thread[i], NULL, thread_func, (void *)&room_num[i])) < 0){
            perror("thread");
            exit(1);
        }
    }

    while (1){
        /*
        1. Nonblocking accpet 호출
            - 만약, 클라이언트가 connect로 접속 했다면, 메뉴 전송  

        2. 채팅 방에서 탈퇴한 사용자가 있는지 확인 (chatinfo[i].returned_users)
            - 있다면, 채팅방에서 대기실로 사용자의 소켓을 이동

        3. (대기실에 있는 사용자만 대상으로) 일정시간 후에 timeout 하는 select 호출
            - readfds 에 변화가 있었다면 FD_ISSET으로 확인하면서 클라이언트의 요청 처리
                (1) 채팅방 정보 요청 -> 채팅방 정보를 사용자에게 전달 (chatinfo[i].new_users)
                (2) 채팅방 참여 -> 해당 채팅방 스레드에 사용자 소켓 정보 전달
                (3) 종료 -> 접속 종료 처리
        */

       // select문 timeout 설정 (1ms)
       sleep(1);

       timeout.tv_sec = 1;
       timeout.tv_usec = 0;

        for (int i = 0; i < 3; i++){
            if (chatinfo[i].returned_users.user_sd > 0){
                printf("[MAIN] 채팅방 탈퇴 사용자 탐지 : %d\n", chatinfo[i].returned_users.user_sd);
                
                // 채팅방에서 대기실로 사용자 소켓 이동 (빈자리 찾기)
                for(int j = 0; j < TOTAL_CLIENTS; j++){
                    if(wait_sock[j] < 0){
                        wait_sock[j] = chatinfo[i].returned_users.user_sd;
                        break;
                    }
                }
                // 나간 소켓 자리 -1 초기화
                chatinfo[i].returned_users.user_sd = -1;
            }
        }

        FD_ZERO(&main_readfds);

        for(int i = 0; i < TOTAL_CLIENTS; i++){
            FD_SET(wait_sock[i], &main_readfds);
        }

        main_ret = select(maxArr(wait_sock, TOTAL_CLIENTS)+1, &main_readfds, NULL, NULL, &timeout);

        switch(main_ret){
            case -1:
                break;
            case 0:
                break;
            default:
                k = 0;
                while (main_ret > 0){
                    if (FD_ISSET(wait_sock[k], &main_readfds)){
                        memset(main_buf, 0, BUFFER_LENGTH);

                        if ((main_length = recv(wait_sock[k], main_buf, BUFFER_LENGTH, 0)) == -1){
                            perror("recv");
                            break;
                        }

                        printf("[MAIN] 사용자 %d 메시지 : %s", wait_sock[k], main_buf);

                        if(strcmp(main_buf, "1\n") == 0){

                            sprintf(main_buf, "<ChatRoom info>\n[ID: 0] Chatroom-0 (%d/5)\n[ID: 1] Chatroom-1 (%d/5)\n[ID: 2] Chatroom-2 (%d/5)\n\n", chatinfo[0].room.user_cnt, chatinfo[1].room.user_cnt, chatinfo[2].room.user_cnt);

                            if(send(wait_sock[k], main_buf, strlen(main_buf), 0) == -1){
                                perror("send");
                                exit(1);
                            }
                        }
                        else if(strcmp(main_buf, "2 0\n") == 0){
                            printf("[MAIN] 사용자 %d가 채팅방 0에 참여합니다.\n", wait_sock[k]);
                            chatinfo[0].new_users.user_sd = wait_sock[k];
                            chatinfo[0].room.user_cnt++;
                            wait_sock[k] = -1;
                        }
                        else if(strcmp(main_buf, "2 1\n") == 0){
                            printf("[MAIN] 사용자 %d가 채팅방 1에 참여합니다.\n", wait_sock[k]);
                            chatinfo[1].new_users.user_sd = wait_sock[k];
                            chatinfo[1].room.user_cnt++;
                            wait_sock[k] = -1;
                        }
                        else if(strcmp(main_buf, "2 2\n") == 0){
                            printf("[MAIN] 사용자 %d가 채팅방 2에 참여합니다.\n", wait_sock[k]);
                            chatinfo[2].new_users.user_sd = wait_sock[k];
                            chatinfo[2].room.user_cnt++;
                            wait_sock[k] = -1;
                        }
                        else if(strcmp(main_buf, "3\n") == 0){
                            printf("[MAIN] %d 사용자와의 접속을 해제합니다.\n", wait_sock[k]);
                            
                            // close client socket
                            wait_sock[k] = -1;
                        }
                        else if(strcmp(main_buf, "0\n") == 0){
                            if(send(wait_sock[k], menu_buf, strlen(menu_buf), 0) == -1){
                                perror("send");
                                exit(1);
                            }
                        }
                        main_ret--;
                    }
                    else{
                        k++;
                    }
                }
                break;
        }
    }
}

void *thread_func(void *arg){
    int c, rn;

    rn = *((int *)arg);

    while (1){
        signal(SIGUSR1,thread_exit);
        /*
        1. 새로운 채팅방 참가자가 있는지 확인 : chatinfo[i].new_users
            - 있다면, 해당 사용자의 소켓 기술자를 chatinfo[i].room로 이동

        2. 채팅방에 참여한 사용자 중에서 메시지를 전송한(send) 사람이 있는지 확인 : select
            - 있다면,
            (1) "quit" 이면, 채팅방에서 탈퇴 처리 :
                해당 사용자의 소켓 기술자를 chatinfo[i].returned_users로 이동
            (2) 그 외 이면, 채팅 메시지 이니까, send한 사용자 이외의 다른 모든 사용자에게 채팅메세지를 전달하기
        */
            
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        FD_ZERO(&thread_readfds);

        // 새로운 채팅방 참가자 확인
        if (chatinfo[rn].new_users.user_sd > 0){
            // 채팅방 빈 소켓 자리 찾기
            for(int i = 0; i < ROOM_CLIENTS; i++){
                if(chatinfo[rn].room.user_sd[i] < 0){
                    chatinfo[rn].room.user_sd[i] = chatinfo[rn].new_users.user_sd;
                    chatinfo[rn].new_users.user_sd = -1;

                    break;
                }
            }
        }

        for(int i = 0; i < ROOM_CLIENTS; i++){
            FD_SET(chatinfo[rn].room.user_sd[i], &thread_readfds);
        }

        thread_ret = select(maxArr(chatinfo[rn].room.user_sd, ROOM_CLIENTS)+1, &thread_readfds, NULL, NULL, &timeout);

        switch(thread_ret){
            case -1:
                perror("select");
            case 0:
                break;
            default:
                c = 0;
                while (thread_ret > 0){
                    if (FD_ISSET(chatinfo[rn].room.user_sd[c], &thread_readfds)){
                        memset(thread_buf, 0, BUFFER_LENGTH);
                        memset(spread_buf, 0, BUFFER_LENGTH);

                        if ((thread_length = recv(chatinfo[rn].room.user_sd[c], thread_buf, BUFFER_LENGTH, 0)) == -1){
                            perror("recv");
                            break;
                        }
                        
                        printf("[Ch. %d] 사용자 %d의 메시지 : %s", rn, chatinfo[rn].room.user_sd[c], thread_buf);

                        if (strcmp(thread_buf, quit_msg) == 0){
                            printf("[Ch. %d] 사용자 %d를 채팅방에서 제거합니다.\n", rn, chatinfo[rn].room.user_sd[c]);

                            chatinfo[rn].returned_users.user_sd = chatinfo[rn].room.user_sd[c];
                            
                            chatinfo[rn].room.user_sd[c] = -1;

                            if(chatinfo[rn].room.user_cnt > 0){
                                chatinfo[rn].room.user_cnt--;
                            }
                            else{ 
                                chatinfo[rn].room.user_cnt = 0;
                            }
                        }
                        else {
                            printf("[Ch. %d] 사용자 %d의 메시지를 전달합니다.\n", rn, chatinfo[rn].room.user_sd[c]);
                            
                            if(chatinfo[rn].room.user_cnt == 1){
                                printf("[Ch. %d] 사용자 %d가 혼자여서 메시지를 전달안합니다.\n", rn, chatinfo[rn].room.user_sd[c]);
                            }
                            else{
                                for(int i = 0; i < 5; i++){
                                    if(chatinfo[rn].room.user_sd[i] > 0){
                                        if(chatinfo[rn].room.user_sd[i] == chatinfo[rn].room.user_sd[c]){ 
                                            continue;
                                        }
                                        else{
                                            sprintf(spread_buf, "[%d] : %s", chatinfo[rn].room.user_sd[c], thread_buf);

                                            if(send(chatinfo[rn].room.user_sd[i], spread_buf, strlen(spread_buf), 0) == -1){
                                                perror("send");
                                                exit(1);
                                            }
                                        }
                                    }
                                }
                            }
                            
                        }
                        thread_ret--;
                    }
                    else{
                        c++;
                    }
                }
                break;
        }
    }
}