#ifndef PACKET_H
#define PACKET_H
// new additions
struct pdu_header{
   uint32_t sequence;
   uint8_t crc[2];
   uint8_t flag;
} __attribute__((packed));

#define HEADER_LEN sizeof(struct pdu_header)
#define MAX_BS 1400
#define MAX_BUFF MAX_BS + HEADER_LEN

void print_buff(uint8_t *buff, int len);
int build_data_pdu(uint8_t *buffer, uint32_t sequence, uint8_t flag, uint8_t *payload, int data_len);
void build_header(uint8_t *buffer, uint32_t sequence, uint8_t flag);
int validate_checksum(uint8_t *buffer, int len);
#endif