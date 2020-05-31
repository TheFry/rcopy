/* Library to manipulate socket table for chat program
 * This is not efficient for large table 
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "safemem.h"
#include "table.h"

static struct table_entry *table;
static size_t size;
static uint32_t window_size; /* window size */
static uint32_t current;
static uint32_t upper;
static uint32_t lower;
static int table_index;
int table_closed;

/* Get memory for the table and init the memory
 */
void init_table(uint32_t given_size){
   struct table_entry *entry;
   int i;
   char empty[MAX_BUFF] = "";

   if(given_size == -1){ given_size = DEFAULT_TABLE_SIZE; }
   /* Get memory and size values */
   size = ENTRY_SIZE * window_size;
   window_size = size / ENTRY_SIZE;
   table = (struct table_entry *)smalloc(size);

 
   for(i = 0; i < window_size; i++){
      entry = &table[i];
      entry->seq = 0;
      entry->pdu_len = 0;
      smemcpy(entry->pdu, empty, MAX_BUFF);
   }

   current = 0;
   lower = 0;
   upper = lower + window_size;
   table_index = 0;
   table_closed = 0;
}


/* Free memory */
void reset_table(){
   if(table != NULL){
      free(table);
   }
   table = NULL;
   size = 0;
}


/* Add packet to window */
int enq(uint32_t seq, uint8_t *pdu, int pdu_len){
   if(table_closed){
      fprintf(stderr, "Error adding seq: %u\tWindow is table_closed\n", seq);
      return -1;
   } 
   /* clear pdu entry */
   smemset(table[table_index].pdu, '\0', pdu_len);

   table[table_index].seq = seq;
   smemcpy(table[table_index].pdu, pdu, pdu_len);
   table_index = (table_index + 1) % window_size;
   current += 1;

   if(current == upper){
      table_closed = 1;
   }
   return 0;
}


/* Slide upper and lower
 * Don't change current
 */
int deq(uint32_t seq, uint32_t rr){
   int seq_table_index;

   if((seq_table_index = get_entry(seq)) == -1){
      fprintf(stderr, "Error getting entry for deq\n");
      return -1;
   }

   /* Sanity check */
   if(rr > upper){
      fprintf(stderr, "RR greater than the upper window limit.\n");
      return -1;
   }

   lower = rr;
   upper = lower + window_size;
   return 0;
}


void print_table(){
   int i;
   struct table_entry *entry;

   printf("\n\nTable:\n");
   for(i = 0; i < window_size; i++){
      entry = &table[i];
      printf("Entry %d\n", i);
      printf("Seq num #: %u\n", entry->seq);
      printf("Printing pdu...\n\n");
      print_buff(entry->pdu, entry->pdu_len);
   }
}


/* Get entry based on sequence number */
int get_entry(uint32_t seq){
   int i = 0;
   for(i = 0; i < window_size; i++){
      if(table[i].seq == seq){
         return i;
      }
   }
   return -1;
}