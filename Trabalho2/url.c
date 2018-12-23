#include <errno.h>
#include <netdb.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SIZE 256
#define PORT 21

#define NORMAL 1;
#define ANON 0;

typedef char url_attr[SIZE];

typedef struct URL {
    url_attr user;
    url_attr password;
    url_attr host;
    url_attr ip;
    url_attr path;
    url_attr filename;
    int port;
} url;

void alloc_url(url *url) {

    memset(url->user, 0, sizeof(url_attr));
    memset(url->password, 0, sizeof(url_attr));
    memset(url->host, 0, sizeof(url_attr));
    memset(url->ip, 0, sizeof(url_attr));
    memset(url->path, 0, sizeof(url_attr));
    memset(url->filename, 0, sizeof(url_attr));
    url->port = PORT;
}

// delimitator delim;

int parse_url(url *url, const char *str) {

    size_t size = strlen(str);
    int mode;
    char aux_url[size], segment[size];

    memset(aux_url, 0, size);
    memset(segment, 0, size);

    if (strstr(str, "@") != NULL) {
        mode = NORMAL;
    } else {
        mode = ANON;
    }

    strcpy(aux_url, str + 6);

    char *tempSize;

    if (mode) {

        memset(aux_url, 0, size);
        strcpy(aux_url, str + 6);

        // SELECT USER;
        tempSize = strchr(aux_url, ':');
        strncpy(segment, aux_url, tempSize - aux_url);
        memcpy(url->user, segment, strlen(segment));

        strcpy(aux_url, aux_url + (tempSize - aux_url) + 1);
        memset(segment, 0, size);

        // SELECT PASSWORD;
        tempSize = strchr(aux_url, '@');
        strncpy(segment, aux_url, tempSize - aux_url);
        memcpy(url->password, segment, strlen(segment));

        strcpy(aux_url, aux_url + (tempSize - aux_url) + 1);
        memset(segment, 0, size);
    }

    // SELECT HOST;
    tempSize = strchr(aux_url, '/');
    strncpy(segment, aux_url, (tempSize - aux_url));
    memcpy(url->host, segment, strlen(segment));

    strcpy(aux_url, aux_url + (tempSize - aux_url) + 1);
    memset(segment, 0, size);

    // SELECT PATH;
    tempSize = strrchr(aux_url, '/');
    strncpy(segment, aux_url, (tempSize - aux_url));
    memcpy(url->path, segment, strlen(aux_url));

    strcpy(aux_url, aux_url + (tempSize - aux_url) + 1);

    // SELECT FILENAME
    memcpy(url->filename, aux_url, strlen(aux_url));

    return 0;
}
