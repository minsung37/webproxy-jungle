#include <stdio.h>
#include "csapp.h"
// Recommended max cache and object sizes
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// You won't lose style points for including this long line in your code
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *endof_hdr = "\r\n";
static const char *connection_key = "Connection";
static const char *user_agent_key= "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

void doit(int connfd);
void parse_uri(char *uri,char *hostname,char *path,int *port);
void build_http_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio);
int connect_endServer(char *hostname,int port,char *http_header);


// proxy sequential
int main(int argc,char **argv)
{
    int listenfd, connfd;
    socklen_t  clientlen;
    char hostname[MAXLINE],port[MAXLINE];

    // generic sockaddr struct which is 28 Bytes.The same use as sockaddr
    struct sockaddr_storage clientaddr;

    if(argc != 2) {
        fprintf(stderr,"usage :%s <port> \n",argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd,(SA *)&clientaddr,&clientlen);

        // print accepted message
        Getnameinfo((SA*)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0);
        printf("Accepted connection from (%s %s).\n",hostname,port);

        // sequential handle the client transaction
        doit(connfd);
        Close(connfd);
    }
    return 0;
}


void doit(int connfd)
{
    // the end server file descriptor
    int end_serverfd;
    char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
    char endserver_http_header [MAXLINE];

    // store the request line arguments
    char hostname[MAXLINE],path[MAXLINE];
    int port;

    // rio is client's rio,server_rio is endserver's rio
    rio_t rio,server_rio;

    // 클라이언트가 보낸 요청 헤더에서 method, uri, version을 가져옴
    // GET http://localhost:5000/home.html HTTP/1.1
    Rio_readinitb(&rio,connfd);
    Rio_readlineb(&rio,buf,MAXLINE);
    sscanf(buf,"%s %s %s",method,uri,version); /*read the client request line*/

    if(strcasecmp(method,"GET")){
        printf("Proxy does not implement the method");
        return;
    }

    // 프록시 서버가 엔드 서버(tiny)로 보낼 정보들을 파싱함
    // hostname -> localhost, path -> /home.html, port -> 8000
    parse_uri(uri,hostname,path,&port);

    // 프록시 서버가 엔드 서버로 보낼 요청 헤더들을 만듦. endserver_http_header가 채워진다.
    build_http_header(endserver_http_header,hostname,path,port,&rio);

    // 프록시 서버와 엔드 서버를 연결함
    end_serverfd = connect_endServer(hostname,port,endserver_http_header);
    // clinetfd connected from proxy to end server at proxy side
    if(end_serverfd<0){
        printf("connection failed\n");
        return;
    }

    // 엔드 서버에 HTTP 요청 헤더를 보냄
    Rio_readinitb(&server_rio,end_serverfd);
    Rio_writen(end_serverfd,endserver_http_header,strlen(endserver_http_header));

    // 엔드 서버로부터 응답 메세지를 받아 클라이언트에 보내줌
    size_t n;
    while((n=Rio_readlineb(&server_rio,buf,MAXLINE))!=0)
    {
        printf("proxy received %d bytes,then send\n",n);
        // connfd -> client와 proxy 연결 소켓
        Rio_writen(connfd,buf,n);
    }
    Close(end_serverfd);
}


void build_http_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio)
{
    char buf[MAXLINE],request_hdr[MAXLINE],other_hdr[MAXLINE],host_hdr[MAXLINE];

    sprintf(request_hdr,requestlint_hdr_format,path);

    // 클라이언트 요청 헤더들에서 Host header와 나머지 header들을 구분해서 넣어줌
    while(Rio_readlineb(client_rio,buf,MAXLINE)>0)
    {
        // EOF, '\r\n' 만나면 끝
        if(strcmp(buf,endof_hdr)==0) break;

        // 호스트 헤더 찾기
        if(!strncasecmp(buf,host_key,strlen(host_key)))
        {
            strcpy(host_hdr,buf);
            continue;
        }
        // 나머지 헤더 찾기
        if(strncasecmp(buf,connection_key,strlen(connection_key))
              && strncasecmp(buf,proxy_connection_key,strlen(proxy_connection_key))
              && strncasecmp(buf,user_agent_key,strlen(user_agent_key)))
        {
            strcat(other_hdr,buf);
        }
    }
    if(strlen(host_hdr)==0)
    {
        sprintf(host_hdr,host_hdr_format,hostname);
    }

    // 프록시 서버가 엔드 서버로 보낼 요청 헤더 작성
    sprintf(http_header,"%s%s%s%s%s%s%s",
            request_hdr,
            host_hdr,
            conn_hdr,
            prox_hdr,
            user_agent_hdr,
            other_hdr,
            endof_hdr);

    return ;
}


inline int connect_endServer(char *hostname,int port,char *http_header){
    char portStr[100];
    sprintf(portStr,"%d",port);
    return Open_clientfd(hostname,portStr);
}


void parse_uri(char *uri,char *hostname,char *path,int *port)
{
    *port = 80; // default port
    char* pos = strstr(uri,"//");  // http://이후의 string들

    pos = pos!=NULL? pos+2:uri;  // http:// 없어도 가능

    // port와 path를 파싱
    char*pos2 = strstr(pos,":");
    if(pos2!=NULL)
    {
        *pos2 = '\0';
        sscanf(pos,"%s",hostname);
        sscanf(pos2+1,"%d%s",port,path); // 포트번호 클라이언트 번호로 변경
    }
    else
    {
        pos2 = strstr(pos,"/");
        if(pos2!=NULL)
        {
            *pos2 = '\0';
            sscanf(pos,"%s",hostname);
            *pos2 = '/';
            sscanf(pos2,"%s",path);
        }
        else
        {
            sscanf(pos,"%s",hostname);
        }
    }
    return;
}