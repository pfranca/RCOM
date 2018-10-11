/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MSG_SIZE 6

#define FLAG 0x7E
#define A 0x03
#define SET 0x03
#define UA 0x07

int fd, res;
int state = 0;
unsigned char buf[MSG_SIZE];
unsigned char l;

volatile int STOP = FALSE;

void printBuffer(char *buff, int size)
{
	printf("\nPrinting buffer: ");
	int i = 0;
	for (i = 0; i < size; i++)
	{
		printf("%02x ", buff[i]);
	}
	printf("\n");
}

int receiveSET()
{
	while (STOP == FALSE)
	{
		res = read(fd, &l, 1);
		printf("\nchar:%02x state:%d\n", l, state);
		switch (state)
		{
		case -1:
			//bzero(buf, sizeof(buf));
			memset(buf, 0, MSG_SIZE);
			if (l == FLAG)
			{
				state = 1;
				buf[0] = l;
			}
			else
				state = 0;
			break;
		case 0:
			if (l == FLAG)
			{
				state = 1;
				buf[0] = l;
			}
			else
				state = -1;
			break;
		case 1:
			if (l == A)
			{
				state = 2;
				buf[1] = l;
			}
			else if (l == FLAG)
				state = 1;
			else
				state = -1;
			break;
		case 2:
			if (l == SET)
			{
				state = 3;
				buf[2] = l;
			}
			else if (l == FLAG)
				state = 1;
			else
				state = -1;
			break;
		case 3:
			if (l == A ^ SET)
			{
				state = 4;
				buf[3] = l;
			}
			else if (l == FLAG)
				state = 1;
			else
				state = -1;
			break;
		case 4:
			if (l == FLAG)
			{
				state = 5;
				buf[4] = l;
				STOP = TRUE;
			}
			else
				state = -1;
			break;
		}
	}
	printf("Message received.\n");
	printBuffer(buf, MSG_SIZE);
	return 0;
}

int sendACK()
{

	memset(buf, 0, MSG_SIZE);

	buf[0] = FLAG;
	buf[1] = A;
	buf[2] = UA;
	buf[3] = A ^ UA;
	buf[4] = FLAG;

	if (res = write(fd, buf, MSG_SIZE) != MSG_SIZE)
	{
		printf("Message not written.\n");
		printf("%d bytes written\n", res);
		return 1;
	}
	else
	{
		printBuffer(buf, MSG_SIZE);
		return 0;
	}
}

int llopen()
{
	return receiveSET() || sendACK();
}

int main(int argc, char **argv)
{
	struct termios oldtio, newtio;

	if ((argc < 2) ||
	    ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
	     (strcmp("/dev/ttyS1", argv[1]) != 0)))
	{
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
	}

	/*
		Open serial port device for reading and writing and not as controlling tty
		because we don't want to get killed if linenoise sends CTRL-C.
	*/

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd < 0)
	{
		perror(argv[1]);
		exit(-1);
	}

	if (tcgetattr(fd, &oldtio) == -1)
	{ /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

	//bzero(&newtio, sizeof(newtio));
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
	newtio.c_cc[VMIN] = 1;  /* blocking read until 1 char received */

	/*
		VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
		leitura do(s) pr�ximo(s) caracter(es)
	*/

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1)
	{
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");

	if (llopen())
		printf("llopen() failed.\n");
	else
		printf("llopen() successful.\n");

	sleep(2);

	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);
	return 0;
}
