#include <stdint.h>
#include <arpa/inet.h>
#include "networks.h"
#include "packet.h"
#include "safemem.h"
#include "checksum.h"
#include "table.h"




void server_parse_packet(uint8_t *buffer, int len, struct conn_info conn){
   uint8_t flag = get_type(buffer, len);

   switch (flag){
      case BAD_PACKET:
         break;
      case RR_FLAG:
         server_process_rr(buffer, len);
         break;
      case SREJ_FLAG:
         server_process_srej(buffer, len, conn);
         break;
      default:
         fprintf(stderr, "Unknown packet type. Ignoring\n");
   }  
}



void rcopy_parse_packet(uint8_t *buff, int len){
   uint8_t flag = get_type(buff, len);

   switch(flag){
      case BAD_PACKET:
         break;
      default:
         printf("\nNot defined\n");
         print_buff(buff, len);
         break;
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


int build_rr(uint8_t *buffer, uint32_t sequence, uint32_t rr){
   build_header(buffer, sequence, RR_FLAG);
   uint8_t *ptr = buffer + HEADER_LEN;

   rr = htonl(rr);
   smemcpy(ptr, &rr, sizeof(rr));
   return ptr + sizeof(rr) - buffer;
}


int build_srej(uint8_t *buffer, uint32_t sequence, uint32_t srej){
   build_header(buffer, sequence, SREJ_FLAG);
   uint8_t *ptr = buffer + HEADER_LEN;
   
   srej = htonl(srej);
   smemcpy(ptr, &srej, sizeof(srej));
   return(ptr + sizeof(srej) - buffer);
}


void server_process_rr(uint8_t *buffer, int len){
   uint8_t *ptr = buffer + HEADER_LEN;
   uint32_t rr;

   smemcpy(&rr, ptr, sizeof(rr));
   rr = ntohl(rr);
   deq(rr);
}


void server_process_srej(uint8_t *buffer, int len, struct conn_info conn){
   uint8_t *ptr = buffer + HEADER_LEN;
   struct table_entry *entry;
   uint32_t srej;

   smemcpy(&srej, ptr, sizeof(srej));
   srej = ntohl(srej);
   entry = get_srej(srej);
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
      printf("Checksum invalid, bad packet\n");
   }else{
      printf("Checksum valid!\n");
   }

   for(i= 0; i < len; i++){
      printf("%02x ", buff[i]);
      if(i == line_break){
         printf("\n");
         line_break += line_break;
      }
   }

   printf("\n\n");
}