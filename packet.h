#ifndef __PACKET_H__
// new additions
struct pdu_header{
   uint32_t sequence;
   uint8_t flag;
   uint16_t length;
   uint16_t checksum;
} __attribute__((packed));

//block size 100 bytes
#define BS 100
#define PDU_LEN sizeof(struct pdu_header)

void print_buff(uint8_t *buff, int len);
int build_pdu(uint8_t *buffer, uint32_t sequence, uint8_t flag, uint8_t *payload, int data_len);
struct pdu_header build_header(uint32_t sequence, uint8_t flag);
#endif