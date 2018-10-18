/*Non-Canonical Input Processing*/

#include "common.c"

int llopen()
{
	return send(SET) || receive(UA);
}

int send_data(char* data)
{
	char buf2[MSG_SIZE + 8];
	memset(buf, 0, MSG_SIZE);


	buf2[0] = FLAG;
	buf2[1] = A;
	buf2[2] = 0;
	buf2[3] = A ^ 0;
	buf2[4] = data[0];
	buf2[5] = data[1];
	buf2[6] = data[2];
	buf2[7] = data[3];
	buf2[8] = data[4];
	buf2[9] = data[5];
	buf2[10] = data[6];
	buf2[11] = data[7];
	buf2[12] = FLAG;
	//printf("%s\t\t%s\n", data, buf2);

	return (res = write(fd, buf, MSG_SIZE+8) != MSG_SIZE+8);
}

int llwrite(char* filename)
{
	FILE* file;
	file = fopen(filename, "r");
	char buf2[8];
	while(!feof(file)) {
		if (fread(buf2, 8, 1, file) != 1)
   		 	perror("fread");
		else {
			if(send_data(buf2)) {
				printf("Failed to send data\n");
				return 1;
			}
		}
	}
	

	return 0;
}

int main(int argc, char **argv)
{
	struct termios oldtio, newtio;
	int i, sum = 0, speed = 0;
	

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
	newtio.c_cc[VMIN] = 1;  /* blocking read until 1 chars received */

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
		if (llwrite(argv[2]))
			printf("\nllwrite() failed.\n");
		else
			printf("\nllwrite() successful.\n");
	}

	sleep(3);

	if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
	{
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}
