#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

int getip(url *url) {
    struct hostent *h;

    h = gethostbyname(url->host);

    if (h == NULL) {
        herror("gethostbyname");
        exit(1);
    }

    char *ip = inet_ntoa(*((struct in_addr *)h->h_addr));

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", ip);

    memcpy(url->ip, ip, strlen(ip));

    return 0;
}
