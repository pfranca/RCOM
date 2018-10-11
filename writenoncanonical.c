/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MSG_SIZE	5

#define FLAG 	0x7E
#define A		0x03
#define SET		0x03
#define UA		0x07


int fd,c, res;
int state = -1;
unsigned char l;
unsigned char bufw[MSG_SIZE];
unsigned char bufr[MSG_SIZE];

volatile int STOP=FALSE;

void printBuff(char *buff, int size){
	printf("\nPrinting buffer: ");
    int i = 0;
      for (i=0; i<size;i++){
        printf("%02x ", buff[i]);
     }
	printf("\n");
}

int sendSET(){
	bzero(bufw,255);

	bufw[0] = FLAG;
	bufw[1] = A;
	bufw[2] = SET;
	bufw[3] = A ^ SET;
	bufw[4] = FLAG;

	size_t length  = sizeof(bufw);

    	res = write(fd,bufw,length);
	printBuff(bufw, length);
   	printf("%d bytes written\n", res);

	if(res!=5)
		return 1;
	else
		return 0;
}

int receiveACK(){
while (STOP == FALSE) {
		res = read(fd, &l, 1);
		switch(state) {
			case -1:
				bzero(bufw, sizeof(bufw));
				state = 0;
				break;
    		case 0:
				if (l == FLAG) { state = 1; bufw[0] = l; }
				else state = -1;
				break;
    		case 1:
				if (l == A) { state = 2; bufw[1] = l; }
				else if (l == FLAG) state = 1;
				else state = -1;
				break;
    		case 2:
				if (l == UA) { state = 3; bufw[2] = l; }
				else if (l == FLAG) state = 1;
				else state = -1;
				break;
    		case 3:
				if (l == A ^ UA) { state = 4; bufw[3] = l; }
				else if (l == FLAG) state = 1;
				else state = -1;
				break;
    		case 4:
				if (l == FLAG) { state = 5; bufw[4] = l; }
				else state = -1;
				break;
			case 5:
				STOP = TRUE;
				printf("ACK received.\n");
				break;
		}
	}

	printBuff(bufw, 5);
	return 0;
}

int llopen(){
	return (sendSET() || receiveACK());
}

int main(int argc, char** argv)
{
    struct termios oldtio,newtio;
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */

	


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
 
	llopen();

	sleep(3);
   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


    close(fd);
    return 0;
}
