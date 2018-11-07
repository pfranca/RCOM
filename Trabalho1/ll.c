#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "ll.h"

int reading = 1;
int state = START_STATE;
int conta = 0;
int alarme = 1;
int fail = 0;
int reading_buffer = 1;
int waiting = 0;
unsigned char ns = 0x00;

void printBuffer(unsigned char * buffer, int length){
	int i = 0;
	for(i = 0; i < length; i++) {
		printf("%x:", buffer[i]);
	}
	printf("\n");
}

void atende() {  // atende alarme

	if(waiting){
		conta++;
		printf("Connection timeout #%d \n", conta);
	}

	if(conta >= 5){
		printf("Connection dropped.\n");
		exit(1);
	}
	reading_buffer = 0;
	alarm(3);
}

int write_su_frame(int fd, unsigned char * buffer){
	return write(fd, buffer, 5);
}
int read_buffer(int fd, unsigned char * buffer, int length){

	unsigned char b[] = {0x00};
	int res = 0;
	int i = 0;
	int run = 1;
	int count = 0;
	reading_buffer = 1;

	while(reading_buffer){

		res = read(fd, b, 1);
		if(res == 0){
			waiting = 1;
		} else {
			waiting = 0;
		}

		if(res > 0 && i < length){
			if (b[0] == FLAG)
				count++;

			if(count == 2)
				reading_buffer = 0;

			buffer[i] = b[0];
			i++;
		}
	}
	if(count == 2)
		return i;
	else
		return -1;
}

int send_rej(int fd, unsigned char nr) {

	unsigned char TR_REJ[5];
	TR_REJ[0] = FLAG;
	TR_REJ[1] = AE;

	if (nr == 0x01){
		TR_REJ[2] = 0x81;
	} else {
		TR_REJ[2] = 0x01;
	}

	TR_REJ[3] = TR_REJ[1]^TR_REJ[2];
	TR_REJ[4] = FLAG;

	return write(fd, TR_REJ, 5);
}

int send_rr(int fd, unsigned char nr) {

	unsigned char TR_RR[5];
	TR_RR[0] = FLAG;
	TR_RR[1] = AE;

	if (nr == 0x00){
		TR_RR[2] = 0x85;
	} else {
		TR_RR[2] = 0x05;
	}

	TR_RR[3] = TR_RR[1]^TR_RR[2];
	TR_RR[4] = FLAG;

	return write(fd, TR_RR, 5);
}

int llopen(int fd, int mode) {

	unsigned char buffer[5];
	bzero(buffer, 5);
	int res = 0;
	unsigned char bcc = 0x00;
	int run = 1;

	(void) signal(SIGALRM, atende);

	unsigned char TR_SET[5];
	TR_SET[0] = FLAG;
	TR_SET[1] = AE;
	TR_SET[2] = CSET;
	TR_SET[3] = TR_SET[1] ^ TR_SET[2];
	TR_SET[4] = FLAG;

	unsigned char TR_UA[5];
	TR_UA[0] = FLAG;
	TR_UA[1] = AE;
	TR_UA[2] = CUA;
	TR_UA[3] = TR_UA[1] ^ TR_UA[2];
	TR_UA[4] = FLAG;

	if(mode == TRANSMITER) {
		res = write_su_frame(fd, TR_SET);
		//alarm(3);

		while (run) {
			switch (state) {
			case START_STATE:

				res = read_buffer(fd, buffer, 5);

				if (buffer[0] == FLAG)
					state = FLAG_RCV_STATE;
			break;
			case FLAG_RCV_STATE:

				if (buffer[1] == AE){
					bcc = buffer[1];
					state = A_RCV_STATE;
				}
			break;
			case A_RCV_STATE:

				if (buffer[2] == CUA){
					bcc = bcc^buffer[2];
					state = C_RCV_STATE;
				}
			break;
			case C_RCV_STATE:

				if (buffer[3] == bcc){
					state = BCC_OK_STATE;
				}
			break;
			case BCC_OK_STATE:

				if (buffer[4] == FLAG)
					state = STOP_STATE;
			break;
			case STOP_STATE:
				run = 0;
				state = START_STATE;
			break;
			}
		}
		printf("Transmitter:\n");
	} else if(mode == RECEIVER) {
		while (run) {
			switch (state) {
			case START_STATE:

				res = read_buffer(fd, buffer, 5);

				if (buffer[0] == FLAG)
					state = FLAG_RCV_STATE;
			break;
			case FLAG_RCV_STATE:
				if (buffer[1] == AE){
					bcc = buffer[1];
					state = A_RCV_STATE;
				}
			break;
			case A_RCV_STATE:
				if (buffer[2] == CSET){
					bcc = bcc^buffer[2];
					state = C_RCV_STATE;
				}
			break;
			case C_RCV_STATE:
				if (buffer[3] == bcc){
					state = BCC_OK_STATE;
				}
			break;
			case BCC_OK_STATE:
				if (buffer[4] == FLAG)
					state = STOP_STATE;
			break;
			case STOP_STATE:
				run = 0;
				state = START_STATE;
			break;
			}
		}
		res = write_su_frame(fd, TR_UA);
		printf("Receiver:\n");
	} else {
		printf("llopen():\nInvalid Input: %d\n", mode);
		return 1;
	}
	return 0;
}

int stuff(unsigned char * buffer, int length){

	int i = 0;
	int new_length = 0;
	unsigned char buf[2*length];

	for(i = 0; i < length; i++){
		if(buffer[i] == FLAG){
			buf[new_length] = ESC;
			new_length++;
			buf[new_length] = 0x5E;
		} else if(buffer[i] == ESC){
			buf[new_length] = ESC;
			new_length++;
			buf[new_length] = 0x5D;
		} else {
			buf[new_length] = buffer[i];
		}
		new_length++;
	}
	for(i = 0; i < new_length; i++){
		buffer[i] = buf[i];
	}
	return new_length;
}

int destuff(unsigned char * buffer, int length){

	int i = 0;
	int new_length = 0;
	unsigned char buf[length];

	for(i = 0; i < length; i++){
		if(buffer[i] == ESC){
			if(buffer[i+1] == 0x5E){
				buf[new_length] = 0x7E;
			}
			if(buffer[i+1] == 0x5D){
				buf[new_length] = 0x7D;
			}
			i++;
		} else {
			buf[new_length] = buffer[i];
		}
		new_length++;
	}
	for(i = 0; i < new_length; i++){
		buffer[i] = buf[i];
	}

	return new_length;
}

unsigned char calc_bcc2(unsigned char *buffer, int length){
	unsigned char bcc2 = buffer[0];
	int i = 0;

	for(i = 0; i < length; i++){
		bcc2 ^= buffer[i];
	}
	return bcc2;
}

int calc_frames(int length){
	int size = 0;
	size = length/MAX_SIZE;

	if(length%MAX_SIZE != 0)
		size++;

	return size;
}

int write_buffer(int fd, unsigned char * buffer, int length){

	state = START_STATE;
	int run = 1;
	int res = 0;
	unsigned char bcc2 = calc_bcc2(buffer, length);
	int i = 0;

	(void) signal(SIGALRM, atende);

	unsigned char temp[length+1];
	bzero(temp, length+1);

	for(i = 0; i < length; i++){
		temp[i] = buffer[i];
	}
	temp[length] = bcc2;

	int new_length = stuff(temp, length+1);

	unsigned char TR_I[5+new_length];
	TR_I[0] = FLAG;
	TR_I[1] = AE;
	TR_I[2] = ns;
	TR_I[3] = TR_I[1]^TR_I[2];

	int count = 4;
	for(i = 0; i < new_length; i++){
		TR_I[count] = temp[i];
		count++;
	}

	TR_I[count] = FLAG;

	res = write(fd, TR_I, new_length + 5);

	unsigned char buf[5];
	unsigned char bcc;

	while (run) {

		switch (state) {
		case START_STATE:

			if(alarme){
				alarm(3);
				alarme = 0;
			}

			bzero(buf, 5);
			bcc = 0x00;

			if(read_buffer(fd, buf, 5) == -1){
				state = RESEND_STATE;
			}

			if (buf[0] == FLAG)
				state = FLAG_RCV_STATE;
			else{
				printf("Incorrect Flag.\n");
				state = FAIL_STATE;
			}
		break;
		case FLAG_RCV_STATE:
			if (buf[1] == AE){
				bcc = buf[1];
				state = A_RCV_STATE;
			} else {
				printf("Incorrect Address.\n");
				state = FAIL_STATE;
			}
		break;
		case A_RCV_STATE:
			bcc = bcc^buf[2];
			if (((ns == 0x00) && (buf[2] == 0x85)) || ((ns == 0x40) && (buf[2] == 0x05))){
				state = C_RCV_STATE;
			} else if((buf[2]==0x81) || (buf[2]==0x01)){
				state = RESEND_STATE;
			} else {
				printf("Incorrect Control.\n");
				state = FAIL_STATE;
			}
		break;
		case C_RCV_STATE:
			if (buf[3] == bcc){
				state = BCC_OK_STATE;
			} else {
				printf("Incorrect BCC.\n");
				state = FAIL_STATE;
			}
		break;
		case BCC_OK_STATE:
			if (buf[4] == FLAG){
				alarm(0);
				alarme = 1;
				state = STOP_STATE;
			}
			else {
				printf("Incorrect Flag.\n");
				state = FAIL_STATE;
			}
		break;
		case STOP_STATE:
			run = 0;
			state = START_STATE;
		break;
		case RESEND_STATE:
			if (write(fd, TR_I, new_length + 5) != -1){
				printf("Resent.\n");
				state = START_STATE;
			} else {
				return -1;
			}
		break;
		case FAIL_STATE:
			printf("Incorrect Response.\n");
			tcflush(fd, TCIOFLUSH);
			usleep(100000);
			state = RESEND_STATE;
		break;
		}
	}
	return (length);
}

int llwrite(int fd, unsigned char * buffer, int length){
	return write_buffer(fd, buffer, length);
}

int read_frame(int fd, unsigned char * buffer){

	srand(time(NULL));

	(void) signal(SIGALRM, atende);

	unsigned char buf[MAX_SIZE_I];
	int i = 0;
	int run = 1;
	int state = START_STATE;
	int new_size = 0;

	unsigned char bcc1 = 0x00;
	unsigned char bcc2 = 0x00;
	unsigned char nr;

	int size = 0;
	int bytes_read = 0;


	while (run) {
		switch (state) {
		case START_STATE:

			if(alarme){
				alarm(3);
				alarme = 0;
			}

			fail++;
			if((fail%20) == 0){
				//state = FAIL_STATE;
				break;
			}

			bzero(buf, MAX_SIZE_I);

			bcc1 = 0x00;
			bcc2 = 0x00;
			size = 0;
			bytes_read = 0;

			bytes_read = read_buffer(fd, buf, MAX_SIZE_I);

			if(bytes_read == -1){
				break;
			}

			size = bytes_read;

			size = destuff(buf, size);

			new_size = 0;
			unsigned char temp_buf[MAX_SIZE];
			bzero(temp_buf, MAX_SIZE);

			for(i = 4; i < size-2;  i++){
				temp_buf[new_size] = buf[i];
				new_size++;
			}

			bcc2 = calc_bcc2(temp_buf, new_size);

			if (buf[0] == FLAG)
				state = FLAG_RCV_STATE;
			else{
				printf("Incorrect Flag.\n");
				state = FAIL_STATE;
			}
			break;
			case FLAG_RCV_STATE:
				if (buf[1] == AE){
					bcc1 = buf[1];
					state = A_RCV_STATE;
				} else{
					printf("Incorrect Address.\n");
					state = FAIL_STATE;
				}
			break;
			case A_RCV_STATE:
				bcc1 = bcc1^buf[2];
				if(buf[2] == 0x00){
					nr = 0x00;
					state = C_RCV_STATE;
				} else if (buf[2] == 0x40){
					nr = 0x01;
					state = C_RCV_STATE;
				} else if(buf[2] == CDISC){
					state = DISC_RCV_STATE;
				} else {
					printf("Incorrect Control.\n");
					state = FAIL_STATE;
				}
			break;
			case C_RCV_STATE:
				if (buf[3] == bcc1){
					state = BCC_OK_STATE;
				} else {
					printf("Incorrect BCC1.\n");
					state = FAIL_STATE;
				}
			break;
			case BCC_OK_STATE:
				if (buf[size-2] == bcc2){
					state = BCC2_OK_STATE;
				} else {
					printf("Incorrect BCC2.\n");
					state = FAIL_STATE;
				}
			break;
			case BCC2_OK_STATE:
				if (buf[size-1] == FLAG){
					for(i = 0; i < new_size;  i++){
						buffer[i] = temp_buf[i];
					}
					printf("OK.\n");
					alarm(0);
					alarme = 1;
					send_rr(fd, nr);
					state = STOP_STATE;
				} else {
					printf("Incorrect Flag.\n");
					state = FAIL_STATE;
				}
			break;
			case STOP_STATE:
				run = 0;
				state = START_STATE;
			break;
			case DISC_RCV_STATE:
				if (buf[3] == bcc1){
					state = DISC_BCC_OK_STATE;
				} else {
					printf("Incorrect BCC.\n");
					state = FAIL_STATE;
				}
			break;
			case DISC_BCC_OK_STATE:
				if (buf[4] == FLAG){

					unsigned char DISC[5];

					DISC[0] = FLAG;
					DISC[1] = AR;
					DISC[2] = CDISC;
					DISC[3] = DISC[1]^DISC[2];
					DISC[4] = FLAG;

					int res = write_su_frame(fd, DISC);

					unsigned char UA[5];

					res = read_buffer(fd, UA, 5);

					if( (UA[0] == FLAG) &&
						(UA[1] == AR) &&
						(UA[2] == CUA) &&
						(UA[3] == UA[1]^UA[2]) &&
						(UA[4] == FLAG)){
						printf("Disconnected.\n");

						reading = 0;
						state = STOP_STATE;
					} else {
						printf("Unsafe Disconnect.\n");
						return 0;
					}
				} else {
					printf("Incorrect Flag.\n");
					state = FAIL_STATE;
				}
			break;
			case FAIL_STATE:
				printf("Failed.\n");
				send_rej(fd, nr);
				tcflush(fd, TCIOFLUSH);
				usleep(100000);
				state = START_STATE;
			break;
			}
		}

		if(reading)
			return new_size;
		else
			return 0;
}

int llread(int fd, unsigned char * buffer){
	return read_frame(fd, buffer);
}

int llclose_transmitter(int fd){

	unsigned char buffer[5];
	int res = 0;
	int run = 1;
	int state = START_STATE;

	unsigned char TR_DISC[5];
	TR_DISC[0] = FLAG;
	TR_DISC[1] = AE;
	TR_DISC[2] = CDISC;
	TR_DISC[3] = TR_DISC[1] ^ TR_DISC[2];
	TR_DISC[4] = FLAG;

	res = write_su_frame(fd, TR_DISC);

	int size = read_buffer(fd, buffer, 5);

	if(size == -1)
		return 1;

	unsigned char bcc = 0x00;
	while (run) {
		switch (state) {
		case START_STATE:
			if (buffer[0] == FLAG)
				state = FLAG_RCV_STATE;
			else {
				printf("Incorrect Flag.\n");
				state = FAIL_STATE;
			}
		break;
		case FLAG_RCV_STATE:
			if (buffer[1] == AR){
				bcc = buffer[1];
				state = A_RCV_STATE;
			} else {
				printf("Incorrect Address.\n");
				state = FAIL_STATE;
			}
		break;
		case A_RCV_STATE:
			if (buffer[2] == CDISC){
				bcc = bcc^buffer[2];
				state = C_RCV_STATE;
			} else {
				printf("Incorrect Control.\n");
				state = FAIL_STATE;
			}
		break;
		case C_RCV_STATE:
			if (buffer[3] == bcc){
				state = BCC_OK_STATE;
			} else {
				printf("Incorrect BCC.\n");
				state = FAIL_STATE;
			}
		break;
		case BCC_OK_STATE:
			if (buffer[4] == FLAG){
				state = STOP_STATE;
			} else {
				printf("Incorrect Flag.\n");
				state = FAIL_STATE;
			}
		break;
		case STOP_STATE:
			run = 0;
			state = START_STATE;
		break;
		case FAIL_STATE:
			printf("Incorrect Frame.\n");
			return 1;
		break;
		}
	}

	unsigned char TR_UA[5];
	TR_UA[0] = FLAG;
	TR_UA[1] = AR;
	TR_UA[2] = CUA;
	TR_UA[3] = TR_UA[1] ^ TR_UA[2];
	TR_UA[4] = FLAG;

	res = write_su_frame(fd, TR_UA);

	printf("Disconnected.\n");

	return 0 ;
}

int llclose_receiver(int fd){
	unsigned char close[5];
	read_frame(fd, close);
	return 0;
}

int llclose(int fd, int mode) {

	if(mode == TRANSMITER){
		return llclose_transmitter(fd);
	} else if(mode == RECEIVER){
		return llclose_receiver(fd);
	} else {
		return 1;
	}
}
