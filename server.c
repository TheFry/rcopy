/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

/* Modified by Luke Matusiak May/June 2020 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "networks.h"
#include "packet.h"
#include "table.h"
#include "safemem.h"
#include "pollLib.h"
#include "cpe464.h"


/* Maximum amount of packets to send before bailing */
#define MAX_SEND 10

void processClient(int socketNum);
void checkArgs(int argc, char *argv[], int *port, double *err_rate);

void process_init(uint8_t *buffer, int len,
								 struct sockaddr_in6 *client, int sock);

void setup_child(struct sockaddr_in6 *client,
					  uint8_t *buffer, int len, int sock);

void send_data(FILE *f, struct sockaddr_in6 *client, uint32_t wsize, uint32_t bs);
void send_data_pdu(FILE *f, uint32_t seq, uint32_t bs, struct sockaddr *addr, int sock);


int main(int argc, char *argv[]){ 
	int socketNum = 0;				
	int portNumber = 0;
	double err_rate = 0;

	checkArgs(argc, argv, &portNumber, &err_rate);
	sendtoErr_init(err_rate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	
	socketNum = udpServerSetup(portNumber);
	processClient(socketNum);

	close(socketNum);
	return 0;
}


void processClient(int socketNum){
	int recv_len = 0; 
	uint8_t buffer[MAX_BUFF] = "";	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	
	printf("Ready for new connections\n");
	buffer[0] = '\0';
	while(1){
		smemset(buffer, '\0', MAX_BUFF);
		recv_len = safeRecvfrom(socketNum, buffer, MAX_BUFF, 0,
									  (struct sockaddr *)&client, &clientAddrLen);
		
		printf("Main: Received message from client with ");
		printIPInfo(&client);
		process_init(buffer, recv_len, &client, socketNum);
	}
}


/* Check for init packet and fork if needed */
void process_init(uint8_t *buffer, int len,
								 struct sockaddr_in6 *client, int sock){
	uint8_t type = get_type(buffer, len);

	if(type != INIT_FLAG){
		fprintf(stderr, "Not an init packet. Ignoring\n\n");
		return;
	}

	setup_child(client, buffer, len, sock);
}


/* Call fork and get a new socket. Contact client through this. */
void setup_child(struct sockaddr_in6 *client,
					  uint8_t *buffer, int len, int sock){

	uint8_t *ptr = buffer + HEADER_LEN;
	uint8_t name_len;
	uint32_t wsize;
	uint32_t bs;
	char filename[MAX_NAME] = "";
	uint8_t send_buffer[MAX_BUFF] = "";
	int buff_len = 0;
	int clientAddrLen = sizeof(struct sockaddr_in6);
	FILE *f;


	/* New process */
	if(sfork()){ return; }

	/* Get filename */
	smemcpy(&name_len, ptr, sizeof(uint8_t));
	ptr += sizeof(uint8_t);
	smemcpy(filename, ptr, name_len);
	ptr += name_len;
	/* Try to open file */
	if((f = sfopen(filename, READ)) == NULL){
		buff_len = build_bad_pdu(send_buffer);

		/* Send error packet. Does not check for response because client
		 * will eventually fail, and the file doesn't even exist. Send the
		 * main server socket so client can resend */
		safeSendto(sock, send_buffer, buff_len,
					  0, (struct sockaddr *)client, clientAddrLen);
		exit(-1);
	}

	/* Get wsize and block size */
	smemcpy(&wsize, ptr, sizeof(wsize));
	wsize = ntohl(wsize);
	ptr += sizeof(wsize);
	smemcpy(&bs, ptr, sizeof(bs));
	bs = ntohl(bs);

	send_data(f, client, wsize, bs);
}


/* Create new socket and send data to client */
void send_data(FILE *f, struct sockaddr_in6 *client, uint32_t wsize, uint32_t bs){
	int sock = socket(AF_INET6, SOCK_DGRAM, 0);
	int timeout = 0;
	int pdu_len = 0;
	int did_recv = 0;
	uint8_t file_data[MAX_BUFF];
	uint8_t pdu[MAX_BUFF] = "";
	uint32_t seq = 0;
	struct sockaddr *addr = (struct sockaddr *)client;


	smemset(file_data, '\0', bs);
	init_table(wsize);
	setupPollSet();
	addToPollSet(sock);

	send_data_pdu(f, seq, bs, addr, sock);
	seq++;
	while(timeout < MAX_SEND){

		if(pollCall(0) > 0){
			server_parse_packet(pdu, pdu_len);
			timeout = 0;
			did_recv = 1;
		}

		/* If closed and didn't recv anything then increment timeout */
		if(table_closed){
			if(!did_recv){
				timeout++;
				continue;
			}
		}
		send_data_pdu(f, seq, bs, addr, sock);	
	}
}


void send_data_pdu(FILE *f, uint32_t seq, uint32_t bs, struct sockaddr *addr, int sock){
	size_t amount = 0;
	uint8_t pdu[MAX_BUFF] = "";
	uint8_t file_data[MAX_BUFF] = "";
	int len;

	if((amount = sfread(file_data, 1, bs, f)) == 0){
		//Go to done function
		fprintf(stderr, "Done with data\n");
		return;
	}
	len = build_data_pdu(pdu, seq, file_data, amount);
	safeSendto(sock, pdu, len, 0, addr, sizeof(struct sockaddr_in6));
	enq(seq, pdu, len);
}


/* Get err rate and port number */
void checkArgs(int argc, char *argv[], int *portNumber, double *err_rate){
	if (argc > 3 || argc < 2){
		fprintf(stderr, "Usage %s error-rate [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	*err_rate = atof(argv[1]);
	if(argc == 3){
		*portNumber = atol(argv[2]);
	}
}