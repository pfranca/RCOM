#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MSG_SIZE 5
#define DATA_SIZE 8

#define FLAG 0x7E
#define A 0x03
#define SET 0x03
#define UA 0x07

int fd, c, res;
int state = 0;
int ns = 1;
unsigned char l;
unsigned char buf[MSG_SIZE];

volatile int STOP = FALSE;

void printBuffer(char *buff, int size)
{
	//printf("\nPrinting buffer: ");
	int i = 0;
	for (i = 0; i < size; i++)
	{
		printf("%02x ", buff[i]);
	}
	printf("\n");
}

int send(int sig)
{

	memset(buf, 0, MSG_SIZE);

	buf[0] = FLAG;
	buf[1] = A;
	buf[2] = sig;
	buf[3] = A ^ sig;
	buf[4] = FLAG;

	if (res = write(fd, buf, MSG_SIZE) != MSG_SIZE)
	{
		printf("\nMessage not written.\n");
		printf("%d bytes written\n", res);
		return 1;
	}
	else
	{
		printf("\nMessage sent:\t\t");
		printBuffer(buf, MSG_SIZE);
		return 0;
	}
}

int receive(int sig)
{
	//bzero(buf, MSG_SIZE);
	memset(buf, 0, MSG_SIZE);
	printf("\nEntering receiving loop\n");
	state = 0;
	STOP = FALSE;
	while (STOP == FALSE)
	{
		res = read(fd, &l, 1);
		printf("char:%02x state:%d\n", l, state);
		switch (state)
		{
		case -1:
			//bzero(buf, MSG_SIZE);
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
			if (l == sig)
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
			if (l == A ^ sig)
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

	printf("Message received:\t");
	printBuffer(buf, MSG_SIZE);
	return 0;
}

