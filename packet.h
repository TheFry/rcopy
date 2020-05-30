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
#define MAX_NAME 100
#define DATA_FLAG 3
#define INIT_FLAG 7
#define BAD_FLAG 8

void print_buff(uint8_t *buff, int len);
int build_data_pdu(uint8_t *buffer, uint32_t seq, uint8_t *payload, int data_len);
void build_header(uint8_t *buffer, uint32_t seq, uint8_t flag);
int build_init_pdu(uint8_t *buffer, uint32_t seq, char *file,
                   uint8_t name_len, uint32_t wsize, uint32_t bs);
int validate_checksum(uint8_t *buffer, int len);
#endif