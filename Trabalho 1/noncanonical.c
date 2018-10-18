/*Non-Canonical Input Processing*/

#include "common.c"

int llopen()
{
	return receive(SET) || send(UA);
}

int receive_data()
{
	memset(buf, 0, MSG_SIZE+DATA_SIZE+1);
	printf("\nEntering receiving loop\n");
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
			if (l == 0)
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
			if (l == A ^ 0)
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
			state++;
			buf[4] = l;
			break;
		case 5:
			state++;
			buf[5] = l;
			break;
		case 6:
			state++;
			buf[6] = l;
			break;
		case 7:
			state++;
			buf[7] = l;
			break;
		case 8:
			state++;
			buf[8] = l;
			break;
		case 9:
			state++;
			buf[9] = l;
			break;
		case 10:
			state++;
			buf[10] = l;
			break;
		case 11:
			state++;
			buf[11] = l;
			break;
		case 12:
			if (l == buf[4]^buf[5]^buf[6]^buf[7]^buf[8]^buf[9]^buf[10]^buf[11])
			{
				state = 13;
				buf[12] = l;
			}
			else if (l == FLAG)
				state = 1;
			else
				state = -1;
			break;
		case 13:
			if (l == FLAG)
			{
				state = 14;
				buf[13] = l;
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


int llread(char* filename)
{
	FILE* file;
	file = fopen(filename, "w");
	while(TRUE) {
		receive_data();
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct termios oldtio, newtio;

	if ((argc < 3) ||
	    ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
	     (strcmp("/dev/ttyS1", argv[1]) != 0)))
	{
		printf("Usage:\t./write SerialPort filename\nex:\t./write /dev/ttyS0 pinguim.gif\n");
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
