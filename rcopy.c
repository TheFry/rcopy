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
#define str(a) #a

void initC(int socketNum, struct sockaddr_in6 *server, struct rcopy_args args);
int getData(char * buffer);
struct rcopy_args checkArgs(int argc, char * argv[]);
void print_args(struct rcopy_args args);


int main (int argc, char *argv[])
{
	int socketNum = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	struct rcopy_args args;

	args = checkArgs(argc, argv);
	sendtoErr_init(args.err_rate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	socketNum = setupUdpClientToServer(&server, args.hostname, args.port);
	initC(socketNum, &server, args);
	close(socketNum);

	return 0;
}

void initC(int socketNum, struct sockaddr_in6 *server, struct rcopy_args args){
	int serverAddrLen = sizeof(struct sockaddr_in6);
	int dataLen = 0; 
	uint8_t pdu[MAX_BUFF] = "";
	uint8_t recv_buff[MAX_BUFF] = "";
	int sent_len = 0;
	int recv_len = 0;
	int i = 0;
	int flag;

	setupPollSet();
	addToPollSet(socketNum);

	dataLen = build_init_pdu(pdu, args.remote, args.wsize, args.bs);
	sent_len = safeSendto(socketNum, pdu, dataLen, 0,
				  (struct sockaddr *)server, serverAddrLen);
	i = 1;

	/* Send filename 10 times max */
	while(i < 10){
		if(pollCall(1) == socketNum){
			recv_len = safeRecvfrom(socketNum, recv_buff, MAX_BUFF,
											0, (struct sockaddr *) server, &serverAddrLen);
			print_buff(recv_buff, recv_len);

			if((flag = get_type(recv_buff, recv_len)) == BAD_FLAG){
				fprintf(stderr, "Remote file doesn't exist on server\n");
         	exit(-1);
			}

			if(flag == DATA_FLAG){
				fprintf(stderr, "I'm ready for data!\n");
				exit(-1);
			}

		}else{
			fprintf(stderr, "No response yet...\n");
			safeSendto(socketNum, pdu, dataLen, 0,
						 (struct sockaddr *)server, serverAddrLen);
			i += 1;
		}
	}
	fprintf(stderr, "Never received data\n");
	exit(-1);
	// print out bytes received
	//ipString = ipAddressToString(server);
	//printf("Server with ip: %s and port %d said it received %s\n", ipString, ntohs(server->sin6_port), buffer);
      

}

int getData(char * buffer)
{
	/* Read in the data */
	buffer[0] = '\0';
	
	printf("Enter the data to send: ");
	scanf("%s" xstr(MAX_BUFF) "[^\n]%*[^\n]", buffer);
	getc(stdin);  // eat the \n
	return((strlen(buffer) + 1));
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