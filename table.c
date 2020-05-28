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
static size_t num_elements;


/* Get memory for the table and init the memory
 */
void init_table(){
   struct table_entry *entry;
   int i;
   char empty[MAX_HANDLE] = "";

   /* Get memory and size values */
   size = ENTRY_SIZE * DEFAULT_TABLE_SIZE;
   num_elements = size / ENTRY_SIZE;
   table = (struct table_entry *)smalloc(size);

 
   for(i = 0; i < num_elements; i++){
      entry = &table[i];
      entry->is_free = FREE;
      entry->socket = -1;
      smemcpy(entry->handle, empty, sizeof(empty));
   }

}


/* Free memory */
void reset_table(){
   if(table != NULL){
      free(table);
   }
   table = NULL;
   size = 0;
}


/* Realloc the table, increasing size by TABLE_INCREMENT
 * Returns pointer to location of new memory
 */
size_t expand_size(){
   struct table_entry *temp;
   int i;
   size_t new_base = 0;
   char empty[MAX_HANDLE] = "";
   

   /* Location of new memory */
   new_base = num_elements;
   
   /* Get memory */
   size = size + (ENTRY_SIZE * TABLE_INCREMENT);
   num_elements = size / ENTRY_SIZE;
   table = (struct table_entry *)srealloc(table, size);

   /* Init new memory */
   for(i = 0; i < TABLE_INCREMENT; i++){
      temp = &table[new_base + i];
      temp->is_free = FREE;
      temp->socket = -1;
      memcpy(temp->handle, empty, sizeof(empty));
   }

   /* Return the start of the new memory */
   return(new_base);
}




/* Check table for conflicts and add handles/sockets
 * Call expand_size if needed .
 * Return -1 if there is a handle collision and exit for sockets
 */
int add_entry(char *handle, int socket){
   int i;
   struct table_entry *entry;
   size_t new_mem;
   
   /* Check if handle/socket already exists */
   for(i = 0; i < num_elements; i++){
      entry = &table[i];
      if(strcmp(handle, entry->handle) == 0){
         fprintf(stderr, "Client %d: Handle already in table\n", socket);
         return -1;
      }
      if(socket == entry->socket){
         fprintf(stderr, "Client %d: Socket already in table\n", socket);
         exit(-1);
      }
   }

   /* Find empty space */
   for(i = 0; i < num_elements; i++){
      entry = &table[i];
      if(entry->is_free == FULL){
         continue;
      }
      sstrcpy(entry->handle, handle);
      entry->socket = socket;
      entry->is_free = FULL;
      return(0);
   }

   /* No empty space, increase table size */
   new_mem = expand_size();
   entry = &table[new_mem];
   sstrcpy(entry->handle, handle);
   entry->socket = socket;
   entry->is_free = FULL;
   return(0);
}


/* Checks table for entry and removes it 
 * If it does not exists, returns -1
 * Does not check for duplicates (there shouldn't be any) 
 */
int remove_entry(int socket){
   int i;
   struct table_entry *entry;
   char empty[MAX_HANDLE] = "";

   if(socket == -1){
      return 0;
   }

   for(i = 0; i < num_elements; i++){
      entry = &table[i];
      if(entry->socket == socket){
         printf("Client %d: Removed from table\n", entry->socket);
         smemcpy(entry->handle, empty, sizeof(entry));
         entry->is_free = FREE;
         entry->socket = -1;
         return 0;
      }
   }

   return(-1);
}


/* Use socket to get handle */
int table_get_handle(int socket, char *handle){
   int i;
   for(i = 0; i < num_elements; i++){
      if(table[i].socket == socket){
         smemcpy(handle, table[i].handle, strlen(table[i].handle));
         return(0);
      }
   }
   return(-1);
}


/* Use handle to get socket */
int table_get_socket(char *handle){
   int i;
   for(i = 0; i < num_elements; i++){
      if((strcmp(handle, table[i].handle)) == 0){
         return(table[i].socket);
      }
   }
   return(-1);
}


void print_table(){
   int i;
   struct table_entry *entry;

   printf("\n\nSocket Table:\n");
   for(i = 0; i < num_elements; i++){
      entry = &table[i];
      printf("Entry %d\n", i);
      printf("\tSocket #: %d\n", entry->socket);
      printf("\tHandle: %s\n", entry->handle);
      printf("\tFree flag: %d\n\n", entry->is_free);
   }
}


uint32_t get_num_elements(){
   int i;
   uint32_t num = 0;

   for(i = 0; i < num_elements; i++){
      if(table[i].is_free == FULL){
         num++;
      }
   }

   return num;
}


void get_entry(size_t index, char *handle){
   if(index >= num_elements){
      fprintf(stderr, "Index too high\n");
      return;
   }

   strcpy(handle, table[index].handle);
}