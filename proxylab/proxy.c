#include "csapp.h"
#include "fdbuf.h"
#include "cache_wr.h"

#define FDBUFFER_SIZE 10
#define MIN_THREAD_CNT 4

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

static fdbuf_t fdbuf;
static cache_wr_t *cache_p;

void doproxy(int toclientfd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
void send_request(int tohostfd, char *hostname, char *path, char *method, rio_t *rp_headers);
void clienterror(int fd, char *errnum, char *shortmsg);
void *workthread(void *vargp);

void doproxy(int toclientfd)
{
    int tohostfd;
    char buf[MAXLINE], uri[MAXLINE], hostname[MAXLINE], 
         port[MAXLINE], path[MAXLINE], method[MAXLINE],
         object[MAX_OBJECT_SIZE];
    char *object_p = object;
    int host_res_size, obj_size = 0;
    rio_t rio_client, rio_host;

    Rio_readinitb(&rio_client, toclientfd);
    Rio_readlineb(&rio_client, buf, MAXLINE);
    sscanf(buf, "%s %s", method, uri);
    parse_uri(uri, hostname, port, path);
    printf("hostname: %s\nport: %s\npath: %s\n", hostname, port, path);


    if (get_object_wr(uri, cache_p, object, &obj_size)) { //find object in proxy
        Rio_writen(toclientfd, object, obj_size);
    } else {
        printf("object not found in cache\n");
        tohostfd = Open_clientfd(hostname, port);
        printf("tohostfd: %d\n", tohostfd);
        Rio_readinitb(&rio_host, tohostfd);
        send_request(tohostfd, hostname, path, method, &rio_client);
        obj_size = 0;
        while ((host_res_size = Rio_readnb(&rio_host, buf, MAXLINE)) > 0) {
            Rio_writen(toclientfd, buf, host_res_size);
            if (obj_size >= 0 && (obj_size += host_res_size) < MAX_OBJECT_SIZE) {
                memcpy(object_p, buf, host_res_size);
                object_p += host_res_size;
            } else {
                printf("object_size = %d\n", obj_size);
                obj_size = -1;
            }
        }
        if (obj_size > 0) {
            printf("start caching\n");
            put_object_wr(uri, object, obj_size, cache_p);
        }
        Close(tohostfd);
    }
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
        *ptr = '\0';
    } else {
        strcpy(path, "/");
    }

    ptr = strchr(uri, ':');
    if (ptr) {
        *ptr++ = '\0';
        strcpy(port, ptr);
    }

    strcpy(hostname, uri);
    sprintf(uri, "%s:%s%s", hostname, port, path);
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
    //printf("------\nRequest headers\n%s------\n", buf);
    Rio_writen(tohostfd, buf, strlen(buf));
}

void clienterror(int fd, char *errnum, char *shortmsg)
{
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.0 %s %s\r\n\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
}

void *workthread(void *vargp)
{
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = fdbuf_pop(&fdbuf);
        doproxy(connfd);
        Close(connfd);
    }
}

int main(int argc, char* argv[])
{
    int portnum;
    int listenfd, connfd, i;
    char clienthost[MAXLINE], clientport[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

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
    
    cache_p = init_cache_wr();
    fdbuf_init(&fdbuf, FDBUFFER_SIZE);
    for (i = 0; i < MIN_THREAD_CNT; i++) {
        Pthread_create(&tid, NULL, workthread, NULL);
    }

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, clienthost, MAXLINE, clientport, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", clienthost, clientport);
        printf("Start proxy\n");
        fdbuf_push(&fdbuf, connfd);
    }
    return 0;
}

