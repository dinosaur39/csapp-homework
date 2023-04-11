#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

void doproxy(int toclientfd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
void send_request(int tohostfd, char *hostname, char *path, char *method, rio_t *rp_headers);
void clienterror(int fd, char *errnum, char *shortmsg);

void doproxy(int toclientfd)
{
    int tohostfd;
    char buf[MAXLINE], uri[MAXLINE], hostname[MAXLINE], 
         port[MAXLINE], path[MAXLINE], method[MAXLINE];
    int len_host_res;
    rio_t rio_client, rio_host;

    Rio_readinitb(&rio_client, toclientfd);
    Rio_readlineb(&rio_client, buf, MAXLINE);
    sscanf(buf, "%s %s", method, uri);
    parse_uri(uri, hostname, port, path);
    printf("hostname: %s\nport: %s\npath: %s\n", hostname, port, path);
    tohostfd = Open_clientfd(hostname, port);
    printf("tohostfd: %d\n", tohostfd);

    Rio_readinitb(&rio_host, tohostfd);
    send_request(tohostfd, hostname, path, method, &rio_client);
    while ((len_host_res = Rio_readnb(&rio_host, buf, MAXLINE)) > 0) {
        Rio_writen(toclientfd, buf, len_host_res);
    }
    Close(tohostfd);
}

void parse_uri(char *uri, char *hostname, char *port, char *path)
{
    const char *http_protocol = "http://";
    const char *https_protocol = "https://";
    char *ptr;
    strcpy(port, "80");
    if (!strncmp(uri, http_protocol, strlen(http_protocol))) {
        uri += strlen(http_protocol);
    } else if (!strncmp(uri, https_protocol, strlen(https_protocol))) {
        strcpy(port, "443");
        uri += strlen(https_protocol);
    } 
    ptr = strchr(uri, '/');
    if (ptr) {
        strcpy(path, ptr); 
    } else {
        strcpy(path, "/");
    }
    *ptr = '\0';
    ptr = strchr(uri, ':');
    if (ptr) {
        *ptr++ = '\0';
        strcpy(port, ptr);
    }
    strcpy(hostname, uri);
}

void send_request(int tohostfd, char *hostname, char *path, char *method, rio_t *rp_headers)
{
    char buf[MAXLINE], header[MAXLINE], name[MAXLINE], host_hdr[MAXLINE];
    char *ptr;
    int header_len;

    strcpy(host_hdr, "Host: ");
    strcat(host_hdr, hostname);
    strcat(host_hdr, "\r\n");
    
    while ((header_len = Rio_readlineb(rp_headers, header, MAXLINE)) > 2) {
        ptr = strchr(header, ':');
        if (!ptr) continue;
        *ptr = '\0';
        strcpy(name, header);
        *ptr = ':';
        if (!strcasecmp(name, "Host")) {
            strcpy(host_hdr, header);
        } else if (!strcasecmp(name, "User-Agent")) {
        } else if (!strcasecmp(name, "Connection")) {
        } else if (!strcasecmp(name, "Proxy-Connection")) {
        } else {
            strcat(buf, header);
        }
    }
    sprintf(buf, "%s %s HTTP/1.0\r\n%s%s%s%s\r\n", method, path
    , host_hdr, user_agent_hdr
    , connection_hdr, proxy_connection_hdr);
    printf("------\nRequest headers\n%s------\n", buf);
    Rio_writen(tohostfd, buf, strlen(buf));
}

void clienterror(int fd, char *errnum, char *shortmsg)
{
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.0 %s %s\r\n\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
}

int main(int argc, char* argv[])
{
    int portnum;
    int listenfd, connfd;
    char clienthost[MAXLINE], clientport[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 0;
    }

    if (Signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        unix_error("mask SIGPIPE error");
    }

    portnum = atoi(argv[1]);
    if (portnum <= 1024 || portnum >= 65536) {
        fprintf(stderr, "error: port number must be greater than 1024 and less than 65536.\n");
        return 0;
    }
    
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, clienthost, MAXLINE, clientport, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", clienthost, clientport);
        printf("Start proxy\n");
        doproxy(connfd);
        Close(connfd);
    }
    return 0;
}

