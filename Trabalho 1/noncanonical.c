/*Non-Canonical Input Processing*/

#include "common.c"

int llopen();
int llread(char *filename);
int llclose();
int receive_data(unsigned char *buf);

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
        printf("llopen() failed.\n");
    else {
        printf("llopen() successful.\n");
        if (llread(argv[2]))
            printf("llread() failed.\n");
        else
            printf("llread() successful.\n");
    }

    sleep(2);

    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
    return 0;
}

int llopen() { return receive_su(SET) || send_su(UA); }

int llread(char *filename) {

    FILE *file;
    file = fopen(filename, "w");
    unsigned char datatmp[MSG_SIZE + DATA_SIZE + 1];
    int i;

    printf("Entering receiving loop:\n");

    while (TRUE) {
        receive_data(datatmp);
        file = fopen(filename, "a");
        for (i = 4; i < 11; i++) {
            fprintf(file, "%c", datatmp[i]);
        }
        fclose(file);
        send_su(UA);
    }
    return 0;
}

int receive_data(unsigned char *buf) {

    memset(buf, 0, MSG_SIZE + DATA_SIZE + 1);

    STOP = FALSE;
    state = 0;
    unsigned char k;

    while (STOP == FALSE) {
        res = read(fd, &k, 1);
        switch (state) {
        case -1:
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
        default:
            buf[state] = k;
            state++;
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

    printBuffer(buf, 14);
    return 0;
}
