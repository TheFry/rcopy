/* Library to manipulate socket table for chat program
 * This is not efficient for large table 
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "safemem.h"
#include "table.h"

struct table_entry *table;
static size_t size;
static uint32_t window_size; /* window size */
static uint32_t current;
static uint32_t upper;
static int table_index;
static uint32_t lower;
int window_closed;


/* Get memory for the table and init the memory
 */
void init_table(uint32_t given_size){
   struct table_entry *entry;
   int i;
   char empty[MAX_BUFF] = "";


   if(given_size == -1){ given_size = DEFAULT_TABLE_SIZE; }
   
   /* Get memory and size values */
   size = ENTRY_SIZE * given_size;
   window_size = size / ENTRY_SIZE;
   printf("Window Size: %d\n", window_size);
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
   window_closed = 0;
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
   if(window_closed){
      fprintf(stderr, "Error adding seq: %u\tWindow is window_closed\n", seq);
      return -1;
   } 
   
   if(window_size == 1){
      window_closed = 1;
      table_index = 0;
   }
   /* clear pdu entry */
   smemset(table[table_index].pdu, '5', pdu_len);

   table[table_index].seq = seq;
   table[table_index].pdu_len = pdu_len;
   smemcpy(table[table_index].pdu, pdu, pdu_len);
   table_index = (table_index + 1) % window_size;
   current += 1;

   if(current == upper){ window_closed = 1; fprintf(stderr, "Window closed\n"); }
   return 0;
}


/* Slide upper and lower
 * Don't change current
 */
void deq(uint32_t rr){
   int seq_table_index;

   if(window_size == 1){
      window_closed = 0;
      return;
   }
   if((seq_table_index = get_entry(rr)) == -1){
      fprintf(stderr, "Error getting entry for deq\n");
      return;
   }

   /* Sanity check */
   if(rr > upper){
      fprintf(stderr, "RR greater than the upper window limit.\n");
      exit(-1);
   }

   lower = rr;
   upper = lower + window_size;
   if(current != upper){
      window_closed = 0;
   }
}


void print_table(){
   int i;
   struct table_entry *entry;

   fprintf(stderr, "\n\nTable:\n");
   for(i = 0; i < window_size; i++){
      entry = &table[i];
      fprintf(stderr, "Entry %d\n", i);
      fprintf(stderr, "Seq num #: %u\n", entry->seq);
      //fprintf(stderr, "Printing pdu...\n\n");
      //print_buff(entry->pdu, entry->pdu_len);
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


/* Return low entry in table */
struct table_entry* get_lowest(){
   return &table[lower % window_size];
}


/* These next functions are used by rcopy client */
struct table_entry* get_entry_struct(uint32_t my_seq){
   int i = 0;
   for(i = 0; i < window_size; i++){
      if(table[i].seq == my_seq){
         return &table[i];
      }
   }
   return NULL;
}


/* Return the index cleared, or -1 on error */
int clear_entry(uint32_t my_seq){
   int i = 0;
   for(i = 0; i < window_size; i++){
      if(table[i].seq == my_seq){
         table[i].seq = 0;
         table[i].pdu_len = 0;
         smemset(table[i].pdu, '\0', MAX_BUFF);
      }
   }
   fprintf(stderr, "Can't find entry %u in the table\n", my_seq);
   return -1;
}


/* Return the index added to, or -1 on error */
int put_entry(uint8_t *buffer, int len, uint32_t my_seq){
   int i = 0;
   for(i = 0; i < window_size; i++){
      /* If already in table, return the index */
      if(table[i].seq == my_seq){ return 1; }

      /* Otherwise, check for free space */
      if(table[i].pdu_len == 0){
         table[i].seq = my_seq;
         table[i].pdu_len = len;
         smemcpy(table[i].pdu, buffer, len);
         return 0;
      }
   }

   fprintf(stderr, "Can't find space in table\n");
   exit(-1);
}

