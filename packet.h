#ifndef PACKET_H
#define PACKET_H
#include "networks.h"
struct pdu_header{
   uint32_t sequence;
   uint8_t crc[2];
   uint8_t flag;
} __attribute__((packed));


#define HEADER_LEN sizeof(struct pdu_header)
#define MAX_BS 1400
#define MAX_WINDOW 1400
#define MAX_BUFF MAX_BS + HEADER_LEN
#define MAX_NAME 100
#define DATA_FLAG 3
#define RR_FLAG 5
#define SREJ_FLAG 6
#define INIT_FLAG 7
#define BAD_FLAG 8
#define CLOSE_FLAG 9

#define BAD_PACKET 0

extern int last_seq;

void print_buff(uint8_t *buff, int len);

int build_data_pdu(uint8_t *buffer, uint32_t seq, uint8_t *payload, size_t data_len);

void server_process_srej(uint8_t *buffer, int len, struct conn_info conn);
int build_srej(uint8_t *buffer, uint32_t sequence, uint32_t srej);
void build_header(uint8_t *buffer, uint32_t seq, uint8_t flag);
int parse_data_pdu(uint8_t *pdu, uint8_t *data, int pdu_len);
int build_init_pdu(uint8_t *buffer, char *file, uint32_t wsize, uint32_t bs);
int validate_checksum(uint8_t *buffer, int len);
int rcopy_parse_packet(uint8_t *buff, int len);
uint8_t get_type(uint8_t *buffer, int len);
int build_bad_pdu(uint8_t *buffer);
int build_rr(uint8_t *buffer, uint32_t sequence, uint32_t rr);
void server_process_rr(uint8_t *buffer, int len, struct conn_info conn);
void server_parse_packet(uint8_t *buffer, int len, struct conn_info conn);
void build_close_pdu(uint8_t *buffer, uint32_t seq);
void server_close(struct conn_info conn);
#endif