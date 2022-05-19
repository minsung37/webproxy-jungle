#include "csapp.h"

void echo(int connfd);

// 서버의 포트 번호를 1번째 인자로 받는다 ./tiny 8000
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    // Accept로 보내지는 client 소켓 구조체
    // sockaddr_storage 구조체: 모든 프로토콜의 주소에 대해 Enough room for any addr
    struct sockaddr_storage clientaddr;                                                                                                             
    char client_hostname[MAXLINE], client_port[MAXLINE];

    // 인자 2개 다 받아야 함.
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port> \n", argv[0]);
        exit(0);
    }

    // 해당 포트 번호에 적합한 듣기 식별자를 만들어 준다
    listenfd = Open_listenfd(argv[1]);

    while (1) {
        // 클라이언트의 연결 요청을 계속 받아서 연결 식별자를 만든다(Important! 길이가 충분히 커서 프로토콜-독립적!)
        clientlen = sizeof(struct sockaddr_storage);
        // 클라이언트와 통신하는 연결 식별자
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
      
        // 클라이언트와 제대로 연결됐다는 것을 출력해준다
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}


void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE]; 
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { //line:netp:echo:eof
	printf("%s\n", buf);
	Rio_writen(connfd, buf, n);
    }
}