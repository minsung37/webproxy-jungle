#include "csapp.h"


// 0번째 인자로 실행 파일, 1번째로 호스트네임, 2번째로 포트 넘버를 받는다. ./tiny 3.39.192.211 8000
int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;
 
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    // 서버와의 연결을 성공한다(connect까지)
    clientfd = Open_clientfd(host, port);
    // 1. 클라이언트 소켓 파일 식별자와 읽기 버퍼 rio를 연결한다.
    Rio_readinitb(&rio, clientfd);

    /* 표준 입력에서 텍스트 줄을 반복적으로 읽는다. */
    /* 2. 표준 입력sdtin에서 MAXLINE만큼 바이트를 가져와 buf에 저장한다. */
    while (Fgets(buf, MAXLINE, stdin) != NULL) {  // 6. EOF 표준 입력을 만나면 종료한다.
        // 3. buf 메모리 안의 strlen(buf) 바이트 만큼의(사실상 모두)를 clientfd로 보낸다.
        Rio_writen(clientfd, buf, strlen(buf));  
        // 4. 서버가 rio에 echo줄을 쓰면 그 rio를 읽어서 읽기 버퍼 buf에 쓴다.
        Rio_readlineb(&rio, buf, MAXLINE);
        // 5. buf에 받아온 값을 표준 출력으로 인쇄한다.
        Fputs(buf, stdout);   
    }
    // 루프가 종료되면 클라이언트 식별자를 닫는다. 서버에 EOF 통지가 전송된다.
    Close(clientfd);
    // 클라이언트 종료
    exit(0);
}