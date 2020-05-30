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

#define MAXBUF 80
#define xstr(a) str(a)
#define str(a) #a

void talkToServer(int socketNum, struct sockaddr_in6 * server);
int getData(char * buffer);
struct rcopy_args checkArgs(int argc, char * argv[]);
void print_args(struct rcopy_args args);


int main (int argc, char *argv[])
{
	int socketNum = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	struct rcopy_args args;

	args = checkArgs(argc, argv);
	print_args(args);
	sendtoErr_init(args.err_rate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	socketNum = setupUdpClientToServer(&server, args.hostname, args.port);
	talkToServer(socketNum, &server);
	
	close(socketNum);

	return 0;
}

void talkToServer(int socketNum, struct sockaddr_in6 * server)
{
	int serverAddrLen = sizeof(struct sockaddr_in6);
	//char * ipString = NULL;
	int dataLen = 0; 
	char buffer[MAX_BS] = "";
	uint8_t pdu[MAX_BUFF] = "";
	int sent_len = 0;
	int i = 0;
	
	buffer[0] = '\0';
	while (buffer[0] != '.')
	{
		dataLen = getData(buffer);
		dataLen = build_init_pdu(pdu, i, "test\0", 5, 5, 500);


   	print_buff(pdu, dataLen);
	
		sent_len = safeSendto(socketNum, pdu, dataLen, 0, (struct sockaddr *) server, serverAddrLen);

		printf("Sent: %d\n", sent_len);
		if(sent_len == 0){
			printf("No data sent, check host/port\n");
		}

		i++;
		//safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) server, &serverAddrLen);
		
		// print out bytes received
		//ipString = ipAddressToString(server);
		//printf("Server with ip: %s and port %d said it received %s\n", ipString, ntohs(server->sin6_port), buffer);
	      
	}
}

int getData(char * buffer)
{
	/* Read in the data */
	buffer[0] = '\0';
	
	printf("Enter the data to send: ");
	scanf("%" xstr(MAXBUF) "[^\n]%*[^\n]", buffer);
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
	args.buff_size = atoi(argv[4]);
	args.err_rate = atof(argv[5]);
	sstrcpy(args.hostname, argv[6]);
	args.port = atoi(argv[7]);

	return args;
}


void print_args(struct rcopy_args args){
	printf("Remote: %s\n", args.remote);
	printf("Local: %s\n", args.local);
	printf("Wsize: %d\n", args.wsize);
	printf("buff_size: %d\n", args.buff_size);
	printf("err_rate: %f\n", args.err_rate);
	printf("Host: %s\n", args.hostname);
	printf("Port: %d\n", args.port);
}





