#include "ftp.c"
#include "getip.c"
#include "url.c"
#include <stdio.h>

void printusage(char *argv0) {
    printf("usage1 Normal: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv0);
    printf("usage2 Anonymous: %s ftp://<host>/<url-path>\n", argv0);
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printusage(argv[0]);
        return 1;
    }

    /* Setup url */
    url url;
    alloc_url(&url);

    if (parse_url(&url, argv[1]))
        return -1;

    if (getip(&url)) {
        printf("ERROR: Cannot find ip to hostname %s.\n", url.host);
        return -1;
    }

    /* Check user */
    char user[SIZE];
    memset(user, 0, SIZE);

    if (strlen(url.user)) {
        strcat(user, url.user);
    } else {
        strcat(user, "anonymous");
    }

    /* Check password */
    char password[SIZE];
    memset(password, 0, SIZE);

    if (strlen(url.password)) {
        strcat(password, url.password);
    } else {
        strcat(password, "anonymous");
    }

    printf("USER: %s\n", user);
    printf("PASSWORD: %s\n", password);
    printf("HOST: %s\n", url.host);
    printf("PATH: %s\n", url.path);
    printf("FILE: %s\n", url.filename);

    /* FTP Client */
    ftp ftp;
    ftp_connect(&ftp, url.ip, url.port);
    ftp_login(&ftp, user, password);
    ftp_cwd(&ftp, url.path);
    ftp_pasv(&ftp);
    ftp_retr(&ftp, url.filename);
    ftp_download(&ftp, url.filename);
    ftp_disconnect(&ftp);

    return 0;
}
