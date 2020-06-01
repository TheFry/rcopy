#ifndef TABLE_H
#define TABLE_H


#include <stdint.h>
#include <stddef.h>
#include "networks.h"
#include "packet.h"

#define DEFAULT_TABLE_SIZE 1
#define ENTRY_SIZE sizeof(struct table_entry)
#define NUM_ELEMENTS(X) (ENTRY_SIZE / (X)) 
#define TABLE_INCREMENT 10
#define FREE 0
#define FULL 1
 /* extra 1 for null terminator */

struct table_entry{
   uint32_t seq;
   uint8_t pdu[MAX_BUFF];
   int pdu_len;
} __attribute__((packed));

extern int table_closed;

void init_table(uint32_t size);
void reset_table();
void print_table();
int get_entry(uint32_t seq);
int enq(uint32_t seq, uint8_t *pdu, int pdu_len);
void deq(uint32_t rr);
void reset_table();


#endif