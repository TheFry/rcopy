#include <stdint.h>
#include <arpa/inet.h>
#include "networks.h"
#include "packet.h"
#include "safemem.h"
#include "checksum.h"
#include "table.h"

int last_seq = -1;  /* Server sets this as the seq number of the eof pdu */

void server_parse_packet(uint8_t *buffer, int len, struct conn_info conn){
   uint8_t flag = get_type(buffer, len);

   switch (flag){
      case BAD_PACKET:
         break;
      case RR_FLAG:
         server_process_rr(buffer, len, conn);
         break;
      case SREJ_FLAG:
         server_process_srej(buffer, len, conn);
         break;
      default:
         fprintf(stderr, "Unknown packet type. Ignoring\n");
   }  
}


int rcopy_parse_packet(uint8_t *buff, int len){
   uint8_t flag = get_type(buff, len);

   switch(flag){
      case BAD_PACKET:
         fprintf(stderr, "Bad packet\n");
         return -1;
      case DATA_FLAG:
         return 0;
      case CLOSE_FLAG:
         return 1;
      default:
         fprintf(stderr, "Not data packet\n");
         print_buff(buff, len);
         return -1;
   }
}


uint8_t get_type(uint8_t *buffer, int len){
   struct pdu_header *header = (struct pdu_header *)buffer;

   if(validate_checksum(buffer, len)){
      /* Bad checksum */
      fprintf(stderr, "Bad packet\n");
      return 0;
   }

   return header->flag;
}


void server_close(struct conn_info conn){
   fprintf(stderr, "Closing sockets and streams.\n");
   fprintf(stderr, "Leaving an orphan :(\nStay safe out there buddy\n");
   fclose(conn.f);
   reset_table();
   close(conn.sock);
   exit(-1);
}


void rcopy_close(struct conn_info conn, uint32_t rr, uint32_t local_seq){
   uint8_t pdu[MAX_BUFF] = "";
   int len;

   fprintf(stderr, "Server sent EOF. Cleaning up...\n");
   fclose(conn.f);
   len = build_rr(pdu, local_seq, rr);
   print_buff(pdu, len);
   safeSendto(conn.sock, pdu, len, 0, conn.addr, conn.addr_len);
   close(conn.sock);
   reset_table();
   exit(0);
}



int build_rr(uint8_t *buffer, uint32_t sequence, uint32_t rr){
   struct pdu_header *header = (struct pdu_header *)buffer;
   uint8_t *ptr = buffer + HEADER_LEN;
   unsigned short chkval = 0;

   build_header(buffer, sequence, RR_FLAG);
   rr = htonl(rr);
   smemcpy(ptr, &rr, sizeof(rr));
   chkval = in_cksum((unsigned short *)buffer, HEADER_LEN + sizeof(rr));
   smemcpy(header->crc, &chkval, sizeof(chkval));
   return HEADER_LEN + sizeof(rr);
}


int build_srej(uint8_t *buffer, uint32_t sequence, uint32_t srej){
   build_header(buffer, sequence, SREJ_FLAG);
   struct pdu_header *header = (struct pdu_header *)buffer;
   uint8_t *ptr = buffer + HEADER_LEN;
   short chkval;

   srej = htonl(srej);
   smemcpy(ptr, &srej, sizeof(srej));
   chkval = in_cksum((unsigned short *)buffer, HEADER_LEN + sizeof(srej));
   smemcpy(header->crc, &chkval, sizeof(chkval));
   return(ptr + sizeof(srej) - buffer);
}


/* Process RR's, close if RR'd the server close packet */
void server_process_rr(uint8_t *buffer, int len, struct conn_info conn){
   uint8_t *ptr = buffer + HEADER_LEN;
   uint32_t rr;
   printf("RR:\n");
   print_buff(buffer, len);
   smemcpy(&rr, ptr, sizeof(rr));
   rr = ntohl(rr);

   /* Just RR'd the final packet response from client */
   if(rr - 1 == last_seq){
      fprintf(stderr, "Client has closed cleanly\n");
      server_close(conn);
   }

   deq(rr);
}


void server_process_srej(uint8_t *buffer, int len, struct conn_info conn){
   uint8_t *ptr = buffer + HEADER_LEN;
   struct table_entry *entry;
   uint32_t srej;

   smemcpy(&srej, ptr, sizeof(srej));
   srej = ntohl(srej);
   entry = get_entry_struct(srej);
   safeSendto(conn.sock, entry->pdu, entry->pdu_len, 0, conn.addr, conn.addr_len);
}


int build_data_pdu(uint8_t *buffer, uint32_t sequence,
                   uint8_t *payload, size_t data_len){

   unsigned short check_val = 0;
   struct pdu_header *header = (struct pdu_header *)buffer;
   uint8_t *ptr = buffer + HEADER_LEN;

   build_header(buffer, sequence, DATA_FLAG);
   smemcpy(ptr, payload, data_len);

   check_val = in_cksum((unsigned short *)buffer, HEADER_LEN + data_len);
   smemcpy(header->crc, &check_val, sizeof(uint8_t) * 2);

   return(HEADER_LEN + data_len);
}


int parse_data_pdu(uint8_t *pdu, uint8_t *data, int pdu_len){
   uint8_t *ptr = pdu + HEADER_LEN;
   int data_len = pdu_len - HEADER_LEN;

   if(data_len <= 0){
      fprintf(stderr, "Empty data packet :(\n");
   }

   smemset(data, '\0', MAX_BUFF);
   smemcpy(data, ptr, data_len);
   return data_len;
}

/* Init packet header:
 * Normal pdu header + name_len + name + wsize + bs
 */
int build_init_pdu(uint8_t *buffer, char *file, uint32_t wsize, uint32_t bs){
   struct pdu_header *header = (struct pdu_header *)buffer;
   unsigned short check_val = 0;
   uint8_t *ptr = buffer + HEADER_LEN;
   uint32_t val = 0;
   int buff_size = 0;
   uint8_t name_len = strlen(file);

   build_header(buffer, 0, INIT_FLAG);
   print_buff(buffer, HEADER_LEN);
   smemcpy(ptr, &name_len, sizeof(name_len));
   ptr += sizeof(name_len);
   smemcpy(ptr, file, name_len);
   ptr += name_len;
   val = htonl(wsize);
   smemcpy(ptr, &val, sizeof(val));
   ptr += sizeof(val);
   val = htonl(bs);
   smemcpy(ptr, &val, sizeof(val));
   ptr += sizeof(val);
   buff_size = (uintptr_t)ptr - (uintptr_t)buffer;
   check_val = in_cksum((unsigned short *)buffer, buff_size);
   smemcpy(header->crc, &check_val, sizeof(uint8_t) * 2);
   return(buff_size);
}


void build_close_pdu(uint8_t *buffer, uint32_t seq){
   unsigned short chk_val = 0;
   struct pdu_header *header = (struct pdu_header *)buffer;

   build_header(buffer, seq, CLOSE_FLAG);
   chk_val = in_cksum((unsigned short *)buffer, HEADER_LEN);
   smemcpy(header->crc, &chk_val, sizeof(chk_val));
}


int build_bad_pdu(uint8_t *buffer){
   struct pdu_header *header = (struct pdu_header *)buffer;
   unsigned short check_val;

   smemset(buffer, '\0', MAX_BUFF);
   header->flag = BAD_FLAG;
   header->sequence = 0;
   check_val = in_cksum((unsigned short *)buffer, HEADER_LEN);
   smemcpy(header->crc, &check_val, sizeof(check_val));
   return HEADER_LEN;
}


void build_header(uint8_t *buffer, uint32_t sequence, uint8_t flag){
   struct pdu_header *pdu;

   smemset(buffer, '\0', MAX_BUFF);
   pdu = (struct pdu_header *)buffer;
   pdu->sequence = htonl(sequence);
   pdu->flag = flag;
   pdu->crc[0] = 0;
   pdu->crc[1] = 0;
}


int validate_checksum(uint8_t *buffer, int len){
   unsigned short check_val;
   check_val = in_cksum((unsigned short *)buffer, len);
   return check_val;
}


/* Print the buffer in hex byte by byte
 * Packet len is in network order
 */
void print_buff(uint8_t *buff, int len){
   int line_break = 70;
   int i;
   
   printf("Buffer Data Length: %u\n", len);

   if((validate_checksum(buff, len))){
      fprintf(stderr, "Checksum invalid, bad packet\n");
   }else{
      fprintf(stderr, "Checksum valid!\n");
   }

   for(i= 0; i < len; i++){
      fprintf(stderr, "%02x ", buff[i]);
      if(i == line_break){
         fprintf(stderr, "\n");
         line_break += line_break;
      }
   }

   fprintf(stderr, "\n\n");
}