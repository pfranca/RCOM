#include "common.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

int fd, c, res;
int state = 0;
int ns = 1;
unsigned char l;
unsigned char buf[MSG_SIZE];
unsigned char su_buf[SU_MSG_SIZE];

volatile int STOP = FALSE;

void printBuffer(char *buff, int size);
int send_su(int sig);
int receive_su(int sig);
int stuffing(unsigned char *buffer_in, int length_in, unsigned char *buffer_out, int length_out);
int destuffing(unsigned char *buffer_in, int length_in, unsigned char *buffer_out, int length_out);
unsigned char calc_bcc(unsigned char *buffer, int length, int it);

void printBuffer(char *buff, int size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        printf("%02x ", buff[i]);
    }
    printf("\n");
}

int send_su(int sig) {

    memset(su_buf, 0, SU_MSG_SIZE);

    su_buf[0] = FLAG;
    su_buf[1] = A;
    su_buf[2] = sig;
    su_buf[3] = A ^ sig;
    su_buf[4] = FLAG;

    if (res = write(fd, su_buf, SU_MSG_SIZE) != SU_MSG_SIZE) {
        printf("Message not written. Bytes written: %d\n", res);
        return 1;
    } else {
        // printf("Message sent:\n");
        // printBuffer(su_buf, SU_MSG_SIZE);
        return 0;
    }
}

int receive_su(int sig) {

    memset(buf, 0, MSG_SIZE);
    state = 0;
    STOP = FALSE;

    while (STOP == FALSE) {

        res = read(fd, &l, 1);
        // printf("char:%02x state:%d\n", l, state);

        switch (state) {
        case -1:
            memset(buf, 0, MSG_SIZE);
            if (l == FLAG) {
                state = 1;
                buf[0] = l;
            } else
                state = 0;
            break;
        case 0:
            if (l == FLAG) {
                state = 1;
                buf[0] = l;
            } else
                state = -1;
            break;
        case 1:
            if (l == A) {
                state = 2;
                buf[1] = l;
            } else if (l == FLAG)
                state = 1;
            else
                state = -1;
            break;
        case 2:
            if (l == sig) {
                state = 3;
                buf[2] = l;
            } else if (l == FLAG)
                state = 1;
            else
                state = -1;
            break;
        case 3:
            if (l == A ^ sig) {
                state = 4;
                buf[3] = l;
            } else if (l == FLAG)
                state = 1;
            else
                state = -1;
            break;
        case 4:
            if (l == FLAG) {
                state = 5;
                buf[4] = l;
                STOP = TRUE;
            } else
                state = -1;
            break;
        }
    }
    // printBuffer(buf, MSG_SIZE);
    return 0;
}

/* Takes a input buffer, stuffs it, copies it to the output buffer and returns the written chars */
int stuffing(unsigned char *buffer_in, int length_in, unsigned char *buffer_out, int length_out) {

    int it_in = 0;
    int it_out = 0;

    if (length_out < (length_in * 2)) {
        printf("The output buffer must be at least 2x bigger than the input buffer for proper stuffing.\n");
        return -1;
    }

    for (it_in = 0; it_in < length_in; it_in++) {
        if (buffer_in[it_in] == FLAG) {
            buffer_out[it_out] = ESC;
            it_out++;
            buffer_out[it_out] = O_FLAG;
        } else if (buffer_in[it_in] == ESC) {
            buffer_out[it_out] = ESC;
            it_out++;
            buf[it_out] = O_ESC;
        } else {
            buf[it_out] = buffer_in[it_in];
        }
        it_out++;
    }

    return it_out;
}

/* Takes a input buffer, destuffs it, copies it to the output buffer and returns the written chars */
int destuffing(unsigned char *buffer_in, int length_in, unsigned char *buffer_out, int length_out) {

    int it_in = 0;
    int it_out = 0;

    if (length_out < length_in) {
        printf("The output buffer must be at least the same size as the input buffer for proper destuffing.\n");
        return -1;
    }

    for (it_in = 0; it_in < length_in; it_in++) {
        if (buffer_in[it_in] == ESC) {
            it_in++;
            switch (buffer_in[it_in]) {
            case O_FLAG:
                buffer_out[it_out] = FLAG;
                break;
            case O_ESC:
                buffer_out[it_out] = ESC;
                break;
            default:
                printf("Expected aditional char after escape.");
                return -1;
            }
        } else {
            buffer_out[it_out] = buffer_in[it_in];
        }
        it_out++;
    }
    return it_out;
}

/* Calculates the bcc for a given buffer and starting positive index*/
unsigned char calc_bcc(unsigned char *buffer, int length, int it) {

    if (it < 0)
        it = 0;

    unsigned char bcc = 0x00;

    for (; it < length; it++) {
        bcc ^= buffer[it];
    }
    return bcc;
}
