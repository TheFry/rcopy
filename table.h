#ifndef TABLE_H
#define TABLE_H


#include <stdint.h>
#include <stddef.h>
#include "networks.h"

#define DEFAULT_TABLE_SIZE 10
#define ENTRY_SIZE sizeof(struct table_entry)
#define NUM_ELEMENTS(X) (ENTRY_SIZE / (X)) 
#define TABLE_INCREMENT 10
#define FREE 0
#define FULL 1
 /* extra 1 for null terminator */

struct table_entry{
   int socket;
   uint8_t is_free;
   char handle[MAX_HANDLE + 1];
} __attribute__((packed));

void init_table();
void reset_table();
int add_entry(char *handle, int socket);
int remove_entry(int socket);
int table_get_handle(int socket, char *handle);
int table_get_socket(char *handle);
void print_table();
uint32_t get_num_elements();
void get_entry(size_t index, char *handle);



#endif