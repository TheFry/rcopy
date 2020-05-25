#include "networks.h"
#include "packet.h"
#include "safemem.h"
#include "checksum.h"


struct pdu_header build_header(uint32_t sequence, uint8_t flag){
   struct pdu_header pdu;
   uint16_t len = sizeof(struct pdu_header);

   pdu.sequence = htonl(sequence);
   pdu.flag = flag;
   pdu.checksum = 0;
   pdu.length = htons(len);
   return pdu;
}


int build_pdu(uint8_t *buffer, uint32_t sequence, uint8_t flag, uint8_t *payload, int data_len){
   struct pdu_header *header = (struct pdu_header *)buffer;
   unsigned short check_val = 0;
   
   header->sequence = htonl(sequence);
   header->flag = flag;
   header->checksum = 0;
   header->length = htons(PDU_LEN + data_len);
   
   smemcpy(buffer + PDU_LEN, payload, data_len);
   
   check_val = in_cksum((unsigned short *)buffer, PDU_LEN + data_len);
   header->checksum = htons(check_val);
   
   print_buff(buffer, PDU_LEN + data_len);
   return(PDU_LEN + data_len);
}

/* Print the buffer in hex byte by byte
 * Packet len is in network order
 */
void print_buff(uint8_t *buff, int len){
   int line_break = 70;
   int i;
   
   printf("Buffer Data Length: %u\n", len);;
   for(i= 0; i < len; i++){
      printf("%02x ", buff[i]);
      if(i == line_break){
         printf("\n");
         line_break += line_break;
      }
   }
   printf("\n");
}