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

#define MAXBUF 80
#define xstr(a) str(a)
#define str(a) #a

void talkToServer(int socketNum, struct sockaddr_in6 * server);
int getData(char * buffer);
int checkArgs(int argc, char * argv[], double *err_rate);


int main (int argc, char *argv[])
 {
	int socketNum = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	int portNumber = 0;
	double err_rate = 0;
	
	portNumber = checkArgs(argc, argv, &err_rate);
	sendtoErr_init(err_rate, DROP_OFF, FLIP_OFF, DEBUG_ON, RSEED_OFF);
	socketNum = setupUdpClientToServer(&server, argv[1], portNumber);
	talkToServer(socketNum, &server);
	
	close(socketNum);

	return 0;
}

void talkToServer(int socketNum, struct sockaddr_in6 * server)
{
	int serverAddrLen = sizeof(struct sockaddr_in6);
	//char * ipString = NULL;
	int dataLen = 0; 
	char buffer[MAXBUF+1];
	uint8_t pdu[PDU_LEN] = "";
	int i = 0;
	
	buffer[0] = '\0';
	while (buffer[0] != '.')
	{
		dataLen = getData(buffer);
		dataLen = build_pdu(pdu, i, 4, (uint8_t *)buffer, dataLen);


		printf("Sent: %d\n", dataLen);
	
		safeSendto(socketNum, pdu, dataLen, 0, (struct sockaddr *) server, serverAddrLen);
		
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

int checkArgs(int argc, char * argv[], double *err_rate){
	int portNumber = 0;
	
	/* check command line arguments  */
	if (argc > 4 || argc < 3){
		printf("Usage: %s host-name port-number [optional error rate between 0 and 1]\n", argv[0]);
		exit(1);
	}

	/* set error rate */
	if(argc == 4){
		*err_rate = strtod(argv[3], NULL);
		
		if(*err_rate <= 0 || *err_rate > 1){
			fprintf(stderr, "Bad error rate");
			exit(1);
		}
	}

	
	/* Checks args and returns port number */
	portNumber = atoi(argv[2]);
	return portNumber;
}





