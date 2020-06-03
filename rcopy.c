// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "gethostbyname.h"
#include "networks.h"
#include "cpe464.h"
#include "packet.h"
#include "table.h"
#include "safemem.h"
#include "pollLib.h"

#define xstr(a) str(a)
#define str(a) 

void initC(int socketNum, struct sockaddr_in6 *server,
															 struct rcopy_args args, FILE *f);
int getData(char * buffer);
struct rcopy_args checkArgs(int argc, char * argv[]);
void print_args(struct rcopy_args args);
FILE* init_file(char *path);
void recv_data(int socketNum, struct sockaddr *addr,
					struct rcopy_args args, uint8_t *buffer, int len, FILE *f);

int main (int argc, char *argv[]){
	int socketNum = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	struct rcopy_args args;
	FILE *local_file;

	args = checkArgs(argc, argv);
	local_file = init_file(args.local);
	sendtoErr_init(args.err_rate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	socketNum = setupUdpClientToServer(&server, args.hostname, args.port);
	initC(socketNum, &server, args, local_file);
	close(socketNum);

	return 0;
}


/* Initialize the connection */
void initC(int socketNum, struct sockaddr_in6 *server,
															 struct rcopy_args args, FILE *f){

	int serverAddrLen = sizeof(struct sockaddr_in6);
	int dataLen = 0; 
	uint8_t pdu[MAX_BUFF] = "";
	uint8_t recv_buff[MAX_BUFF] = "";
	int recv_len = 0;
	int i = 0;
	int flag = 0;
	struct sockaddr *addr = (struct sockaddr *)server;
	setupPollSet();
	addToPollSet(socketNum);

	dataLen = build_init_pdu(pdu, args.remote, args.wsize, args.bs);

	safeSendto(socketNum, pdu, dataLen, 0, addr, serverAddrLen);
	i = 1;

	/* Send filename 10 times max */
	while(i < 10){
		if(pollCall(1000) == socketNum){
			recv_len = safeRecvfrom(socketNum, recv_buff, MAX_BUFF,
											0, addr, &serverAddrLen);

			if((flag = get_type(recv_buff, recv_len)) == BAD_FLAG){
				fprintf(stderr, "Remote file doesn't exist on server\n");
         	exit(-1);
			}

			if(flag == DATA_FLAG){
				fprintf(stderr, "I'm ready for data!\n");
				recv_data(socketNum, addr, args, recv_buff, recv_len, f);
			}

			/* Bad packet, resend */
			safeSendto(socketNum, pdu, dataLen, 0, addr, serverAddrLen);
			i += 1;

		}else{
			fprintf(stderr, "No response\n");
			safeSendto(socketNum, pdu, dataLen, 0, addr, serverAddrLen);
			i += 1;
		}
	}

	fprintf(stderr, "Never received data\n");
	exit(-1); 
}



void recv_data(int socketNum, struct sockaddr *addr,
					struct rcopy_args args, uint8_t *pdu, int len, FILE *f){
	
	uint8_t data_buff[MAX_BUFF] = "";
	int data_len = 0;
	int addr_len = sizeof(struct sockaddr_in6);
	uint32_t seq = 1;
	uint32_t waiting_on[args.wsize];
	uint32_t expected = 0;
	init_table(args.wsize);
	struct pdu_header *header = (struct pdu_header *)pdu;
	int done = 0;
	int i = 0;
	if(ntohl(header->sequence) == expected){
		data_len = parse_data_pdu(pdu, data_buff, len);
		sfwrite(data_buff, 1, data_len, f);
		expected++;
		len = build_rr(data_buff, seq, expected);
		printf("RR\n");
		print_buff(data_buff, len);
		seq++;
	}

	while(!done){
		if(pollCall(1) == socketNum){
			len = safeRecvfrom(socketNum, pdu, MAX_BUFF,
									 0, addr, &addr_len);

			if(rcopy_parse_packet(pdu, len)){ continue; }  /*Bad packets, ignore*/
			
			if(ntohl(header->sequence) == expected){
				data_len = parse_data_pdu(pdu, data_buff, len);

				sfwrite(data_buff, 1, data_len, f);
				expected++;
				len = build_rr(data_buff, seq, expected);
				printf("RR\n");
				print_buff(data_buff, len);
				safeSendto(socketNum, data_buff, len, 0, addr, addr_len);
				seq++;
			}
		}else{
			printf("%d\n", i);
			if(i == 3){
				fclose(f);
				exit(-1);
			}
			i++;
		}

	}
}



FILE* init_file(char *path){
	FILE* f = sfopen(path, "w");
	if(f == NULL){
		exit(-1);
	}
	return f;
}


struct rcopy_args checkArgs(int argc, char * argv[]){
	struct rcopy_args args;
	
	/* check command line arguments  */
	if (argc != 8){
		printf("Usage: %s remote-filename\nlocal-filename\nwindow-size\nbuffer-size\nerror-rate\nhost-name\nport-number\n", argv[0]);
		exit(-1);
	}

	sstrcpy(args.remote, argv[1]);
	sstrcpy(args.local, argv[2]);
	args.wsize = atoi(argv[3]);
	args.bs = atoi(argv[4]);
	args.err_rate = atof(argv[5]);
	sstrcpy(args.hostname, argv[6]);
	args.port = atoi(argv[7]);

	return args;
}


void print_args(struct rcopy_args args){
	printf("Remote: %s\n", args.remote);
	printf("Local: %s\n", args.local);
	printf("Wsize: %d\n", args.wsize);
	printf("buff_size: %d\n", args.bs);
	printf("err_rate: %f\n", args.err_rate);
	printf("Host: %s\n", args.hostname);
	printf("Port: %d\n", args.port);
}