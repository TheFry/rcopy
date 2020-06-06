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

void process_new_clients(int socketNum, double err_rate);
void checkArgs(int argc, char *argv[], int *port, double *err_rate);

void process_init(uint8_t *buffer, int len,
						struct sockaddr_in6 *client, int sock, double err_rate);

void setup_child(struct sockaddr_in6 *client,
					  uint8_t *buffer, int len, int sock, double err_rate);

void send_data(struct conn_info conn);
int send_data_pdu(struct conn_info conn, uint32_t seq);
void resend_lowest(struct conn_info conn);
void server_close(struct conn_info conn);

static int timeout;
int main(int argc, char *argv[]){ 
	int socketNum = 0;				
	int portNumber = 0;
	double err_rate = 0;

	checkArgs(argc, argv, &portNumber, &err_rate);
	sendtoErr_init(err_rate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);
	
	socketNum = udpServerSetup(portNumber);
	process_new_clients(socketNum, err_rate);

	close(socketNum);
	return 0;
}


void process_new_clients(int socketNum, double err_rate){
	int recv_len = 0; 
	uint8_t buffer[MAX_BUFF] = "";	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	
	printf("Ready for new connections\n");
	buffer[0] = '\0';
	setupPollSet();
	addToPollSet(socketNum);
	while(1){

		if(pollCall(1) >= 0){
			smemset(buffer, '\0', MAX_BUFF);
			recv_len = safeRecvfrom(socketNum, buffer, MAX_BUFF, 0,
										  (struct sockaddr *)&client, &clientAddrLen);
			
			printf("Main: Received message from client with ");
			printIPInfo(&client);
			process_init(buffer, recv_len, &client, socketNum, err_rate);
		}
	}
}


/* Check for init packet and fork if needed */
void process_init(uint8_t *buffer, int len,
					   struct sockaddr_in6 *client, int sock, double err_rate){
	uint8_t type = get_type(buffer, len);

	if(type != INIT_FLAG){
		//fprintf(stderr, "Not an init packet. Ignoring\n\n");
		return;
	}

	setup_child(client, buffer, len, sock, err_rate);
}


/* Call fork, get filename/wsize/bs */
void setup_child(struct sockaddr_in6 *client,
					  uint8_t *buffer, int len, int sock, double err_rate){

	uint8_t send_buffer[MAX_BUFF] = "";
	uint8_t *ptr = buffer + HEADER_LEN;
	uint8_t name_len;
	uint32_t wsize;
	uint32_t bs;
	char filename[MAX_NAME] = "";
	struct conn_info conn;
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
	if((f = sfopen(filename, "r")) == NULL){
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
	conn.wsize = ntohl(wsize);
	ptr += sizeof(wsize);
	smemcpy(&bs, ptr, sizeof(bs));
	conn.bs = ntohl(bs);
	conn.addr = (struct sockaddr *)client;
	conn.addr_len = sizeof(struct sockaddr_in6);
	conn.f = f;

	/* Remove main server socket */
	removeFromPollSet(sock);
	send_data(conn);
}


/* Create new socket and send data to client */
void send_data(struct conn_info conn){
	timeout = 0;
	int pdu_len = 0;
	uint8_t file_data[MAX_BUFF];
	uint8_t pdu[MAX_BUFF] = "";
	uint32_t seq = 0;
	int done = 0;

	/* Child uses new socket to talk to client */
	conn.sock = socket(AF_INET6, SOCK_DGRAM, 0);
	
	init_table(conn.wsize);
	setupPollSet();
	addToPollSet(conn.sock);

	while(timeout < MAX_SEND){
		smemset(file_data, '\0', MAX_BUFF);
		smemset(pdu, '\0', MAX_BUFF);
		if(!window_closed && !done){			/* window_closed defined in table.c */
			timeout = 0;

			/* Try to send data, dont increment seq if nothing sent */
			if(send_data_pdu(conn, seq)){ done = 1; }
			seq++;
			if(pollCall(0) > 0){
				pdu_len = safeRecvfrom(conn.sock, pdu, MAX_BUFF,
											  0, conn.addr, &conn.addr_len);
				server_parse_packet(pdu, pdu_len, conn);
				continue;
			}
		}else{
			if(pollCall(1000) > 0){
				pdu_len = safeRecvfrom(conn.sock, pdu, MAX_BUFF, 0,
											  conn.addr, &conn.addr_len);
				server_parse_packet(pdu, pdu_len, conn); 
				timeout = 0;

			/* None recieved, window closed. Resend lowest to prevent deadlock */
			}else{
				resend_lowest(conn);
				timeout++;
			}
		}	
	}
	fprintf(stderr, "Maximum timeout exceeded\n");
	server_close(conn);
}


void resend_lowest(struct conn_info conn){
	struct table_entry *entry = get_lowest();
	int amount = 0;

	if(entry->pdu_len == 0){
		return;
	}
	if((amount = safeSendto(conn.sock, entry->pdu, entry->pdu_len, 0,
										conn.addr, conn.addr_len)) != entry->pdu_len){

		fprintf(stderr, "Error resending packet\n");
		exit(-1);
	}
}


/* Read file and send data pdu to rcopy.
 * If EOF, send a close pdu to rcopy.
 * Set global close seq number which is used to determine when the final
 * packet has been rr'd
 */
int send_data_pdu(struct conn_info conn, uint32_t seq){
   size_t amount = 0;
   uint8_t pdu[MAX_BUFF] = "";
   uint8_t file_data[MAX_BUFF] = "";
   int len;
   int done = 0;

   if((amount = sfread(file_data, 1, conn.bs, conn.f)) == 0){
   	/* EOF */
      fprintf(stderr, "Done reading data\n");
      build_close_pdu(pdu, seq);
      len = HEADER_LEN;
      done = 1;
      last_seq = seq;
   }else{
   	len = build_data_pdu(pdu, seq, file_data, amount);
   } 

   //print_buff(pdu, len);
   safeSendto(conn.sock, pdu, len, 0, conn.addr, conn.addr_len);
   enq(seq, pdu, len);

   if(done){ return 1; }
   return 0;
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