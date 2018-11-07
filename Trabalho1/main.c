/* Non-Canonical Input Processing */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include "al.c"

//#define BAUDRATE B19200
#define BAUDRATE B38400
//#define BAUDRATE B115200
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

int main(int argc, char** argv) {
	int fd, c, res;
	struct termios oldtio, newtio;
	char buf[255];
	int i, sum = 0, speed = 0;

	if ((argc < 2)
			|| ((strcmp("/dev/ttyS0", argv[1]) != 0)
					&& (strcmp("/dev/ttyS1", argv[1]) != 0))) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
	}

	/*
	 Open serial port device for reading and writing and not as controlling tty
	 because we don't want to get killed if linenoise sends CTRL-C.
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

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
	newtio.c_cc[VMIN] = 0; /* blocking read until 5 chars received */

	/*
	 VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
	 leitura do(s) proximo(s) caracter(es)
	 */

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");

	/* Main configuration done */

	int run = 1;
	while(run){
		printf("Prima:\n1 Para enviar.\n2 Para receber.\n3 Para mudar a Baudrate.\n");
		int option;
		scanf("%d", &option);

		switch(option){
		case 1 :
			printf("Enviar ficheiro.\n");
			if(!send_file(fd)){
				printf("Enviado.\n");
				run = 0;
			} else {
				printf("Falhou o envio.\n");
			}
			break;
		case 2 :
			printf("Receber ficheiro.\n");
			if(!receive_file(fd)){
				printf("Recebido.\n");
				run = 0;
			} else {
				printf("Falhou a receccao.\n");
			}
			break;

		case 3 :
			printf("Intruduza a Baudrate (ex: \"38400\" para B38400).\n");
			char *p, s[128];
			int n;
			while (fgets(s, sizeof(s), stdin)) {
				n = strtol(s, &p, 10);
				if (p == s || *p != '\n') {
					printf("Please enter an integer: ");
				} else break;
			}
			//checkBaurate(n);
			break;
		default :
			printf("Opcao invalida:\n");
			break;
		}
	}
	printf("Operacao concluida.\n");

	/* Restoring old termios structure */
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}
