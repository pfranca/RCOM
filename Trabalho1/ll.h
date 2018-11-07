#define FLAG 0x7E
#define AE 0x03
#define AR 0x01
#define CSET 0x03
#define CUA 0x07
#define ESC 0x7D
#define CDISC 0x0B

#define TRANSMITER 1
#define RECEIVER 2

#define START_STATE 0
#define FLAG_RCV_STATE 1
#define A_RCV_STATE 2
#define C_RCV_STATE 3
#define BCC_OK_STATE 4
#define STOP_STATE 5
#define READ_STATE 6
#define BCC2_RCV_STATE 7
#define BCC2_OK_STATE 8
#define DISC_RCV_STATE 9
#define DISC_BCC_OK_STATE 10
#define FAIL_STATE 11
#define RESEND_STATE 12

#define TRUE 1
#define FALSE 0

#define MAX_SIZE 256
#define MAX_SIZE_I ((2*MAX_SIZE)+7)
