#include <math.h>
#include "ll.c"

//#define IMG_SIZE 10968
#define PACK_MAX_SIZE 4096

#define TSIZE 0
#define TNAME 1

double starting_time;

double calc_miliseconds(struct timespec curr_time) {
	return (curr_time.tv_sec * 1000.0) + (floor((curr_time.tv_nsec / 1000000.0)*100)/100.0);
}

double get_time(){

	struct timespec curr_time;
	if((clock_gettime(CLOCK_MONOTONIC, &curr_time)) == -1){
		perror("Failed to get_time");
		exit(1);
	}

	return calc_miliseconds(curr_time);
}

double elapsed_time(){
	return get_time() - starting_time;
}

//points to end of file, gets the valeu pointed with ftell, set puts the cursor back in the begining of the file
int calc_file_size(FILE* file){
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

int convert_int_to_hex(unsigned int value, unsigned char * buffer){
	buffer[0] = (value >> 24) & 0xFF;
	buffer[1] = (value >> 16) & 0xFF;
	buffer[2] = (value >> 8) & 0xFF;
	buffer[3] = value & 0xFF;
	return 0;
}

unsigned int convert_hex_to_int(unsigned char * buffer){
	unsigned int value = 0;
	value = buffer[0] << 24;
	value = value | (buffer[1] << 16);
	value = value | (buffer[2] << 8);
	value = value | buffer[3];
	return value;
}

//calc number of packets we're gonna send
int calc_packets(int length){
	int size = 0;
	size = length/(MAX_SIZE-4);

	if(length%(MAX_SIZE-4) != 0) //adiciona mais um pacote ou conforme o tamanho (pode nao bater certo)
		size++;

	return size;
}

int send_data_packets(int fd, unsigned char * buffer, int length){

	int size = calc_packets(length);
	int total = 0;
	int res = 0;
	int num_seq = 0;

	unsigned char PK_D[MAX_SIZE];
	bzero(PK_D, MAX_SIZE);

	PK_D[0] = 0x01;
	PK_D[1] = num_seq;
	PK_D[2] = (size >> 16) & 0xFF;
	PK_D[3] = size & 0xFF;


	int i = 0;
	int j = 0;
	int k = 0;

	if(size == 1){ //se for so um pacote de dados nao temos a certeza do tamanho
		i = 4;
		for(j = 0; j < length; j++){
			PK_D[i] = buffer[j];
			i++;
		}
		return llwrite(fd, PK_D, length+4);
	}else {
		for(i = 0; i < size-1; i++){

			PK_D[1] = num_seq;
			num_seq++;

			for(j = 4; j < MAX_SIZE; j++){
				PK_D[j] = buffer[k];
				k++;
			}

			res = llwrite(fd, PK_D, MAX_SIZE);
			if(res != -1){
				total += res;
			} else {
				return -1;
			}
			printf("Sent: %d/%d\n", i+1, size);
		}

		//envio do ultimo pacote
		PK_D[1] = num_seq;
		num_seq++;

		for(j = 4; j < (length%(MAX_SIZE-4))+4; j++){ //ate nr bytes que faltam
			PK_D[j] = buffer[k];
			k++;
		}

		res = llwrite(fd, PK_D, (length%(MAX_SIZE-4))+4);
		if(res != -1){
			total += res;
		} else {
			return -1;
		}
		printf("Sent: %d/%d\n", i+1, size);
		return total;
	}
	return -1;
}

int send_file(int fd){

	unsigned char path[PACK_MAX_SIZE];
	bzero(path, PACK_MAX_SIZE);

	printf("Introduza o nome do ficheiro:\n");
	scanf("%s", path);

	FILE* file = fopen(path, "r");
	int fd_image = open(path, O_RDONLY);
	unsigned int size = calc_file_size(file);

	printf("File size: %d\n", size);

	unsigned int size_size = 4;
	unsigned char value_size[size_size];

	convert_int_to_hex(size, value_size);

	unsigned char value_name[PACK_MAX_SIZE];

	int size_name = sprintf(value_name, "%s", path); //tamanho nome

	int pk_c_size = 1+2+size_size; //star+T+L+V(size_size)

	//criar pacote controlo com tamanho e nome do ficheiro
	unsigned char PK_C[pk_c_size];
	bzero(PK_C, pk_c_size);
	PK_C[0] = 0x02;
	PK_C[1] = TSIZE;
	PK_C[2] = size_size;

	int i = 0;
	for(i = 3; (i-3) < size_size; i++){
		PK_C[i] = value_size[i-3];
	}

	PK_C[i] = TNAME;
	i++;
	PK_C[i] = size_name;
	i++;

	int j = 0;
	for(j = 0; j < size_name; j++){
		PK_C[i] = value_name[j];
		i++;
	}

	pk_c_size = i;

	llopen(fd, TRANSMITER);

	starting_time = get_time();

	llwrite(fd, PK_C, pk_c_size); //evia pacote controlo (start)

	unsigned char buffer[size]; 
	int res = read(fd_image, buffer, size); // le imagem

	send_data_packets(fd, buffer, size); //envia pacotes de dados

	PK_C[0] = 0x03; // muda pacote de controlo start para end
	llwrite(fd, PK_C, pk_c_size); //envia pacote de controlo (end)


	printf("Total time:%-10.2f", elapsed_time());

	llclose(fd, TRANSMITER);

	return 0;
}

int receive_file(int fd) {

	int res = 0;
	int i = 0;
	int j = 0;
	int k = 0;

	llopen(fd, RECEIVER);

	starting_time = get_time();

	unsigned char PK_C_S[7+2+PACK_MAX_SIZE];
	bzero(PK_C_S, 7+2+PACK_MAX_SIZE);
	res = llread(fd, PK_C_S);

	int file_size = convert_hex_to_int(PK_C_S+3);

	int packets = calc_packets(file_size);

	unsigned char file[file_size];
	bzero(file, file_size);

	unsigned char PK_D[MAX_SIZE];
	bzero(PK_D, MAX_SIZE);

	int total = 0;

	for(i = 0; i < packets; i++){
		res = llread(fd, PK_D);
		total += res;
		for(j = 4; j < res; j++){
			file[k] = PK_D[j];
			k++;
		}
	}

	int name_size  = PK_C_S[8];

	unsigned char name_value[name_size+1];
	bzero(name_value, name_size+1);

	j = 9;
	for(i = 0; i < name_size; i++){
		name_value[i] = PK_C_S[j];
		j++;
	}

	name_value[name_size] = '\0';

	int fout = open(name_value, O_RDWR | O_CREAT | O_TRUNC, 0644);
	write(fout, file, k);

	unsigned char PK_C_E[7+2+PACK_MAX_SIZE];
	bzero(PK_C_E, 7+2+PACK_MAX_SIZE);

	res = llread(fd, PK_C_E);

	printf("Total time:%-10.2f", elapsed_time());

	llclose(fd, RECEIVER);

	return 0;
}
