#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFF_SIZE 1024

typedef struct FTP {
    int control_socket_fd;
    int data_socket_fd;
} ftp;

int ftp_send(ftp *ftp, const char *buffer, size_t size) {
    int res;

    if ((res = write(ftp->control_socket_fd, buffer, size)) <= 0) {
        printf("WARNING: Nothing was send.\n");
        return 1;
    }

    return 0;
}

int ftp_read(ftp *ftp, char *buffer, size_t size) {
    FILE *fp = fdopen(ftp->control_socket_fd, "r");
    int flag = 0;

    do {
        memset(buffer, 0, size);
        buffer = fgets(buffer, size, fp);
        printf("%s", buffer);
        flag = !('1' <= buffer[0] && buffer[0] <= '5') || buffer[3] != ' ';
    } while (flag);

    return 0;
}

int socket_open(const char *srv_addr, int srv_port) {

    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(srv_addr); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(srv_port);            /*server TCP port must be network byte ordered */

    /*open an TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket().");
        exit(0);
    }

    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect().");
        exit(0);
    }

    return sockfd;
}

int ftp_connect(ftp *ftp, const char *ip, int port) {
    int socketfd = socket_open(ip, port);
    char buffer[BUFF_SIZE];

    if (socketfd < 0) {
        printf("ERROR: Cannot connect socket.\n");
        return 1;
    }

    ftp->control_socket_fd = socketfd;
    ftp->data_socket_fd = 0;

    ftp_read(ftp, buffer, sizeof(buffer));

    return 0;
}

int ftp_login(ftp *ftp, const char *user, const char *password) {
    char buffer[BUFF_SIZE];

    sprintf(buffer, "USER %s\r\n", user);
    if (ftp_send(ftp, buffer, strlen(buffer))) {
        printf("ERROR: username send failure.\n");
        return 1;
    }

    ftp_read(ftp, buffer, sizeof(buffer));

    memset(buffer, 0, strlen(buffer));

    sprintf(buffer, "PASS %s\r\n", password);
    if (ftp_send(ftp, buffer, strlen(buffer))) {
        printf("ERROR: password send failure.\n");
        return 1;
    }

    ftp_read(ftp, buffer, sizeof(buffer));

    return 0;
}

int ftp_cwd(ftp *ftp, const char *path) {
    char buffer[BUFF_SIZE];

    sprintf(buffer, "CWD %s\r\n", path);
    if (ftp_send(ftp, buffer, strlen(buffer))) {
        printf("ERROR: Cannot set CWD to path.\n");
        return 1;
    }

    ftp_read(ftp, buffer, sizeof(buffer));

    return 0;
}

int ftp_pasv(ftp *ftp) {
    char buffer[BUFF_SIZE] = "PASV\r\n";
    char ip[BUFF_SIZE];

    if (ftp_send(ftp, buffer, strlen(buffer))) {
        printf("ERROR: Cannot set passive mode.\n");
        return 1;
    }

    ftp_read(ftp, buffer, sizeof(buffer));

    int ip_addr_1, ip_addr_2, ip_addr_3, ip_addr_4, port_1, port_2;
    sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip_addr_1, &ip_addr_2, &ip_addr_3, &ip_addr_4,
           &port_1, &port_2);
    sprintf(ip, "%d.%d.%d.%d", ip_addr_1, ip_addr_2, ip_addr_3, ip_addr_4);

    int expected_port = (port_1 * 256) + port_2;

    printf("IP: %s\n", ip);
    printf("PORT: %d\n", expected_port);

    ftp->data_socket_fd = socket_open(ip, expected_port);

    return 0;
}

int ftp_retr(ftp *ftp, const char *filename) {
    char buffer[BUFF_SIZE];

    sprintf(buffer, "RETR %s\r\n", filename);
    if (ftp_send(ftp, buffer, strlen(buffer))) {
        printf("ERROR: Cannot send filename.\n");
        return 1;
    }

    ftp_read(ftp, buffer, sizeof(buffer));

    return 0;
}

int ftp_download(ftp *ftp, const char *filename) {
    FILE *file;
    int res;

    if (!(file = fopen(filename, "w"))) {
        printf("ERROR: Cannot open file.\n");
        return 1;
    }

    char buffer[BUFF_SIZE];
    while ((res = read(ftp->data_socket_fd, buffer, sizeof(buffer)))) {
        if (res < 0) {
            printf("ERROR: Nothing was received from data socket fd.\n");
            return 1;
        }

        if ((res = fwrite(buffer, res, 1, file)) < 0) {
            printf("ERROR: Cannot write data in file.\n");
            return 1;
        }
    }

    fclose(file);
    close(ftp->data_socket_fd);

    memset(buffer, 0, strlen(buffer));
    ftp_read(ftp, buffer, sizeof(buffer));

    return 0;
}

int ftp_disconnect(ftp *ftp) {
    char buffer[BUFF_SIZE];

    sprintf(buffer, "QUIT\r\n");
    if (ftp_send(ftp, buffer, strlen(buffer))) {
        printf("ERROR: Cannot send QUIT command.\n");
        return 1;
    }

    memset(buffer, 0, strlen(buffer));
    ftp_read(ftp, buffer, sizeof(buffer));

    if (ftp->control_socket_fd)
        close(ftp->control_socket_fd);

    return 0;
}
