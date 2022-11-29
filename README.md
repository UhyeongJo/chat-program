# chat-program
Network Socket Programming

## ▪︎ Build

```
$ make clean
$ make
```

## ▪︎ Run
```
$ ./server.out
$ ./c1-disp.out
$ ./c1-term.out
$ ./c2-disp.out
$ ./c2-term.out
$ ./c3-disp.out
$ ./c3-term.out
$ ./c4-disp.out
$ ./c4-term.out
```

## ▪︎ Server

* 출력 전용
* 스레드를 사용한 동시 동작 서버 프로그램
* 서버 최대 허용 인원 : 15명 / 각 채팅방 최대 인원 : 5명 / 최대 채팅방 수 : 3개   

---

### Server Func.

* 메뉴 정보 클라이언트에 전달
```console
<MENU>
1. 채팅방 목록 보기
2. 채팅방 참여하기 (사용법 : 2 <채팅방 번호>)
3. 프로그램 종료
(0을 입력하면 메뉴가 다시 표시됩니다)
```
* 클라이언트에 대한 로그 출력
  * 사용자 접속 및 접속 해제
  * 사용자 채팅방 개설 / 참여 / 탈퇴   
* 시그널 이벤트 `(ctrl + c)` 발생 시 안정적으로 자원 반납 후 종료


## ▪︎ Client
### Monitor Terminal

* 출력 전용
* 서버가 전송하는 정보에 대한 메시지 / 사용자가 입력한 채팅 메시지가 출력되는 터미널

---

### Input Terminal

* 입력 전용 
* 사용자의 채팅 메시지를 입력 받는 터미널
* `quit` 입력시 채팅방을 탈퇴하고 대기실로 복귀
