#include "networks.h"
#include "packet.h"
#include "safemem.h"
#include "checksum.h"



int build_data_pdu(uint8_t *buffer, uint32_t sequence, uint8_t flag, uint8_t *payload, int data_len){
   unsigned short check_val = 0;
   struct pdu_header *header = (struct pdu_header *)buffer;

   build_header(buffer, sequence, flag);
   smemcpy(buffer + HEADER_LEN, payload, data_len);
   
   check_val = in_cksum((unsigned short *)buffer, HEADER_LEN + data_len);
   smemcpy(header->crc, &check_val, sizeof(uint8_t) * 2);

   return(HEADER_LEN + data_len);
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
      printf("Checksum invalid, bad packet\n\n");
      return;
   }

   printf("Checksum valid!\n");
   for(i= 0; i < len; i++){
      printf("%02x ", buff[i]);
      if(i == line_break){
         printf("\n");
         line_break += line_break;
      }
   }

   printf("\n\n");
}