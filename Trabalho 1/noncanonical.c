/*Non-Canonical Input Processing*/

#include "common.c"

int llopen() { return receive(SET) || send(UA); }

int receive_data(unsigned char *buf) {

    memset(buf, 0, MSG_SIZE + DATA_SIZE + 1);
    printf("\nEntering receiving loop\n");
    STOP = FALSE;
    state = 0;
    unsigned char k;

    while (STOP == FALSE) {

        res = read(fd, &k, 1);
        printf("char:%02x state:%d \n", k, state);

        switch (state) {
        case -1:
            // bzero(buf, MSG_SIZE);
            memset(buf, 0, MSG_SIZE);
            if (k == FLAG) {
                state = 1;
                buf[0] = k;
            } else
                state = 0;
            break;
        case 0:
            if (k == FLAG) {
                state = 1;
                buf[0] = k;
            } else
                state = -1;
            break;
        case 1:
            if (k == A) {
                state = 2;
                buf[1] = k;
            } else if (k == FLAG)
                state = 1;
            else
                state = -1;
            break;
        case 2:
            if (k == 0) {
                state = 3;
                buf[2] = k;
            } else if (k == FLAG)
                state = 1;
            else
                state = -1;
            break;
        case 3:
            if (k == A ^ 0) {
                state = 4;
                buf[3] = k;
            } else if (k == FLAG)
                state = 1;
            else
                state = -1;
            break;
        case 4:
            state++;
            buf[4] = k;
            break;
        case 5:
            state++;
            buf[5] = k;
            break;
        case 6:
            state++;
            buf[6] = k;
            break;
        case 7:
            state++;
            buf[7] = k;
            break;
        case 8:
            state++;
            buf[8] = k;
            break;
        case 9:
            state++;
            buf[9] = k;
            break;
        case 10:
            state++;
            buf[10] = k;
            break;
        case 11:
            state++;
            buf[11] = k;
            break;
        case 12:
            if (k == buf[4] ^ buf[5] ^ buf[6] ^ buf[7] ^ buf[8] ^ buf[9] ^ buf[10] ^ buf[11]) {
                state = 13;
                buf[12] = k;
            } else if (k == FLAG)
                state = 1;
            else
                state = -1;
            break;
        case 13:
            if (k == FLAG) {
                state = 14;
                buf[13] = k;
                STOP = TRUE;
            } else
                state = -1;
            break;
        }
    }

    printf("Message received:\n");
    printBuffer(buf, MSG_SIZE + DATA_SIZE + 1);

    // strncpy(ret, buf, sizeof(buf));
    // printBuffer(ret, MSG_SIZE + DATA_SIZE +1);

    return 0;
}

int llread(char *filename) {

    FILE *file;
    file = fopen(filename, "w");
    unsigned char datatmp[MSG_SIZE + DATA_SIZE + 1];

    while (TRUE) {
        receive_data(datatmp);
        printBuffer(datatmp, MSG_SIZE + DATA_SIZE + 1);
        fwrite(datatmp[4], 1, 1, file);
        fwrite(datatmp[5], 1, 1, file);
        fwrite(datatmp[6], 1, 1, file);
        fwrite(datatmp[7], 1, 1, file);
        fwrite(datatmp[8], 1, 1, file);
        fwrite(datatmp[9], 1, 1, file);
        fwrite(datatmp[10], 1, 1, file);
        fwrite(datatmp[11], 1, 1, file);
        // printBuffer(buf, MSG_SIZE+DATA_SIZE+1);
        send(UA);
    }
    return 0;
}

int main(int argc, char **argv) {

    struct termios oldtio, newtio;

    if ((argc < 3) || ((strcmp("/dev/ttyS0", argv[1]) != 0) && (strcmp("/dev/ttyS1", argv[1]) != 0) &&
                       (strcmp("/dev/ttyS2", argv[1]) != 0))) {
        printf("Usage:\t./write SerialPort filename\nex:\t./write /dev/ttyS0 "
               "pinguim.gif\n");
        exit(1);
    }

    /*
            Open serial port device for reading and writing and not as
       controlling tty because we don't want to get killed if linenoise sends
       CTRL-C.
    */

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror(argv[1]);
        exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    // bzero(&newtio, sizeof(newtio));
    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;  /* blocking read until 1 char received */

    /*
            VTIME e VMIN devem ser alterados de forma a proteger com um
       temporizador a leitura do(s) pr�ximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    if (llopen())
        printf("\nllopen() failed.\n");
    else {
        printf("\nllopen() successful.\n");
        if (llread(argv[2]))
            printf("\nllread() failed.\n");
        else
            printf("\nllread() successful.\n");
    }

    sleep(2);

    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
    return 0;
}
