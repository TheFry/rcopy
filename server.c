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

void processClient(int socketNum);
int checkArgs(int argc, char *argv[]);
void server_parse_packet(uint8_t *buffer, int len, struct sockaddr_in6 *client);
void setup_child(struct sockaddr_in6 *client, uint8_t *buffer, int len);

int main (int argc, char *argv[]){ 
	int socketNum = 0;				
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);
	socketNum = udpServerSetup(portNumber);
	processClient(socketNum);

	close(socketNum);
	
	return 0;
}


void processClient(int socketNum)
{
	int recv_len = 0; 
	uint8_t buffer[MAX_BUFF] = "";	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	
	printf("Ready for new connections\n");
	buffer[0] = '\0';
	while(1){
		smemset(buffer, '\0', MAX_BUFF);
		recv_len = safeRecvfrom(socketNum, buffer, MAX_BUFF, 0, (struct sockaddr *) &client, &clientAddrLen);
		
		printf("Received message from client with ");
		printIPInfo(&client);
		server_parse_packet(buffer, recv_len, &client);

		//just for fun send back to client number of bytes received
		//sprintf(buffer, "bytes: %d", dataLen);
		//safeSendto(socketNum, buffer, strlen(buffer)+1, 0, (struct sockaddr *) & client, clientAddrLen);
	}
}


/* Check for init packet and fork if needed */
void server_parse_packet(uint8_t *buffer, int len, struct sockaddr_in6 *client){
	uint8_t type = get_type(buffer, len);

	if(type != INIT_FLAG){
		fprintf(stderr, "Not an init packet. Ignoring\n\n");
		return;
	}

	setup_child(client, buffer, len);
}


void setup_child(struct sockaddr_in6 *client, uint8_t *buffer, int len){
	uint8_t *ptr = buffer + HEADER_LEN;
	uint8_t name_len;
	uint32_t seq_num;
	uint32_t wsize;
	uint32_t bs;
	char filename[MAX_NAME] = "";
	uint8_t send_buffer[MAX_BUFF] = "";
	int buff_len = 0;
	int socketNum;
	int clientAddrLen = sizeof(struct sockaddr_in6);
	if(sfork()){ return; }
	socketNum = socket(AF_INET6, SOCK_DGRAM, 0);

	smemcpy(&name_len, ptr, sizeof(uint8_t));
	ptr += sizeof(uint8_t);
	smemcpy(filename, ptr, name_len);
	ptr += name_len;

	if(sfopen(filename, READ) == NULL){
		buff_len = build_bad_pdu(send_buffer);
		print_buff(send_buffer, buff_len);
		safeSendto(socketNum, send_buffer, buff_len,
					  0, (struct sockaddr *)client, clientAddrLen);
		exit(-1);
	}

}


int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}