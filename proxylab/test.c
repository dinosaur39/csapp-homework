#include <string.h>
#include <stdio.h>

#define MAX 1024

void parse_uri(char *uri, char *hostname, char *port, char *path)
{
    const char *http_protocol = "http://";
    const char *https_protocol = "https://";
    char *ptr;
    strcpy(port, "80");
    if (!strncmp(uri, http_protocol, strlen(http_protocol))) {
        uri += strlen(http_protocol) ;
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

int main()
{
  char hostname[MAX], port[MAX], path[MAX], uri[MAX];
  while (1) {
  printf("input a url: ");
    scanf("%s", uri);
    printf("url input: %s\n", uri);
    parse_uri(uri, hostname, port, path);
    printf("hostname: %s\n", hostname);
    printf("port: %s\n", port);
    printf("path: %s\n\n", path);
  }
  return 0;
}