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

void initC(struct conn_info conn, struct rcopy_args args);
int getData(char * buffer);
struct rcopy_args checkArgs(int argc, char * argv[]);
void print_args(struct rcopy_args args);
FILE* init_file(char *path);
void recv_data(struct conn_info conn, uint8_t *pdu, int len);
void rcopy_send_rr(uint32_t seq, uint32_t rr, struct conn_info conn);
void rcopy_send_srej(uint32_t seq, uint32_t expected, struct conn_info conn);
void rcopy_write_data(uint8_t *buffer, int len, struct conn_info conn);
uint32_t write_window(uint32_t expected, struct conn_info conn);
void handle_first_packet(uint8_t *pdu, int pdu_len, uint32_t *pdu_seq,
												uint32_t *expected, struct conn_info conn);
int check_srejs(uint32_t srej, uint32_t *srejs, uint32_t wsize);

int main (int argc, char *argv[]){	
	struct conn_info conn;		
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	struct rcopy_args args;

	args = checkArgs(argc, argv);
	conn.f = init_file(args.local);
	conn.sock = setupUdpClientToServer(&server, args.hostname, args.port);
	conn.addr = (struct sockaddr *)&server;
	sendtoErr_init(args.err_rate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	initC(conn, args);
	return 0;
}


/* Initialize the connection */
void initC(struct conn_info conn, struct rcopy_args args){
	int dataLen = 0; 
	uint8_t pdu[MAX_BUFF] = "";
	uint8_t recv_buff[MAX_BUFF] = "";
	int recv_len = 0;
	int i = 0;
	int flag = 0;

	setupPollSet();
	addToPollSet(conn.sock);

	conn.addr_len = sizeof(struct sockaddr_in6);
	conn.wsize = args.wsize;
	conn.bs = args.bs;
	dataLen = build_init_pdu(pdu, args.remote, args.wsize, args.bs);

	safeSendto(conn.sock, pdu, dataLen, 0, conn.addr, conn.addr_len);
	i = 0;

	/* Send filename 10 times max */
	while(i < 10){
		if(pollCall(1000) == conn.sock){
			recv_len = safeRecvfrom(conn.sock, recv_buff, MAX_BUFF,
											0, conn.addr, &conn.addr_len);

			if((flag = get_type(recv_buff, recv_len)) == BAD_FLAG){
				fprintf(stderr, "Remote file doesn't exist on server\n");
         	exit(-1);
			}

			if(flag == DATA_FLAG){
				fprintf(stderr, "I'm ready for data!\n");
				recv_data(conn, recv_buff, recv_len);
			}

			/* Bad packet, resend */
			safeSendto(conn.sock, pdu, dataLen, 0, conn.addr, conn.addr_len);
			i += 1;

		}else{
			fprintf(stderr, "No response\n");
			safeSendto(conn.sock, pdu, dataLen, 0, conn.addr, conn.addr_len);
			i += 1;
		}
	}

	fprintf(stderr, "Never received data\n");
	exit(-1); 
}


void recv_data(struct conn_info conn, uint8_t *pdu, int len){
	uint8_t data_buff[MAX_BUFF] = "";
	int data_len = 0;
	uint32_t seq = 1;
	uint32_t expected = 0;
	init_table(conn.wsize);
	struct pdu_header *header = (struct pdu_header *)pdu;
	int done = 0;
	int type;
	int srej_sent = 0;
	uint32_t pdu_seq = ntohl(header->sequence);
	handle_first_packet(pdu, len, &pdu_seq, &expected, conn);
	seq = pdu_seq;
	pdu_seq = 0;

	while(!done){
		if(pollCall(10000) == conn.sock){
			len = safeRecvfrom(conn.sock, pdu, MAX_BUFF,
									 0, conn.addr, &conn.addr_len);
			/* bad pdu */
			if((type = rcopy_parse_packet(pdu, len)) == -1){ continue; }
			data_len = parse_data_pdu(pdu, data_buff, len);
			pdu_seq = ntohl(header->sequence);
			
			if(pdu_seq < expected){
				rcopy_send_rr(seq, expected, conn);

			}else if(pdu_seq > expected){
				put_entry(data_buff, data_len, pdu_seq);
				if(!srej_sent){
					rcopy_send_srej(seq, expected, conn);
					srej_sent = 1;
				}
			}else{
				if(type == 1){
					rcopy_close(conn, ntohl(header->sequence) + 1, seq);
				}
				srej_sent = 0;
				sfwrite(data_buff, 1, data_len, conn.f);
				expected++;
				expected = write_window(expected, conn);
				rcopy_send_rr(seq, expected, conn);
				seq++;
			}
		}else{
			fprintf(stderr, "Timeout, Exiting...\n");
			exit(-1);
		}
	}
}


int check_srejs(uint32_t srej, uint32_t *srejs, uint32_t wsize){
	int i;

	for(i = 0; i < wsize; i++){
		if(srejs[i] == srej){
			return 1;
		}
	}
	return 0;
}

void handle_first_packet(uint8_t *pdu, int pdu_len, uint32_t *pdu_seq,
												uint32_t *expected, struct conn_info conn){
	uint32_t seq = 1;
	uint8_t data_buff[MAX_BUFF] = "";
	int data_len = parse_data_pdu(pdu, data_buff, pdu_len);

	if(*pdu_seq < *expected){
		fprintf(stderr, "Initial seq is less than 1\n");

	}else if(*pdu_seq > *expected){
		if(!put_entry(pdu, data_len, *pdu_seq)){
			rcopy_send_rr(*expected, seq, conn);
		}
	}else{
		sfwrite(data_buff, 1, data_len, conn.f);
		*expected += 1;
		rcopy_send_rr(seq, *expected, conn);
		seq++;
		*pdu_seq = seq;
	}

}


uint32_t write_window(uint32_t expected, struct conn_info conn){
	int done = 0;
	int i;
	struct table_entry *entry;

	while(done < conn.wsize){
		for(i = 0; i < conn.wsize; i++){
			entry = &table[i];

			/* Found in table */
			if(entry != NULL && entry->seq == expected){
				sfwrite(entry->pdu, 1, entry->pdu_len, conn.f);
				clear_entry(entry->seq);
				expected++;
				break;
			}
		}
		done++;
	}
	return expected;
}


void rcopy_send_srej(uint32_t seq, uint32_t expected, struct conn_info conn){
	uint8_t buffer[MAX_BUFF] = "";
	int len;

	len = build_srej(buffer, seq, expected);
	//printf("SREJ\n");
	//print_buff(buffer, len);
	safeSendto(conn.sock, buffer, len, 0, conn.addr, conn.addr_len);
}


void rcopy_send_rr(uint32_t seq, uint32_t rr, struct conn_info conn){
	uint8_t buffer[MAX_BUFF] = "";
	int len;

	len = build_rr(buffer, seq, rr);
	//printf("RR\n");
	//print_buff(buffer, len);
	safeSendto(conn.sock, buffer, len, 0, conn.addr, conn.addr_len);
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
	if(args.wsize <= 0 || args.wsize >= MAX_WINDOW){
		fprintf(stderr, "Bad window size\n");
		exit(-1);
	}
	if(args.bs <= 0 || args.bs >= MAX_BS){
		fprintf(stderr, "Bad block size\n");
		exit(-1);
	}
	if(args.err_rate < 0 || args.err_rate > 1){
		fprintf(stderr, "Bad error rate\n");
	}
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