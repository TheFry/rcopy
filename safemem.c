/* Wrapper that checks return values of standard memory libs **/

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "safemem.h"
#include "packet.h"


void *smemcpy(void *dest, const void *src, size_t n){
   void *ptr = NULL;

   ptr = memcpy(dest, src, n);
   
   if(ptr == NULL){
      perror("memcpy");
      exit(-1);
   }

   return ptr;
}


void *smalloc(size_t size){
   void *ptr = NULL;
   ptr = malloc(size);
   
   if(ptr == NULL){
      perror("malloc");
      exit(-1);
   }

   return ptr;
}


void *smemset(void *s, int c, size_t n){
   void *ptr = NULL;
   ptr = memset(s, c, n);

   if(ptr == NULL){
      perror("memset");
      exit(-1);
   }

   return ptr;
}


void *srealloc(void *ptr, size_t size){
   void *new_ptr = NULL;
   new_ptr = realloc(ptr, size);

   if(new_ptr == NULL){
      perror("realloc");
      exit(-1);
   }

   return new_ptr;
}


/* Written by professor Smith */
void * sCalloc(size_t nmemb, size_t size)
{
   void * returnValue = NULL;
   if ((returnValue = calloc(nmemb, size)) == NULL)
   {
      perror("calloc");
      exit(-1);
   }
   return returnValue;
}


void sstrcpy(char *dest, const char *src){
   void *ptr = NULL;
   ptr = strcpy(dest, src);
   if(ptr == NULL){
      perror("strcpy");
      exit(-1);
   }
}


size_t sstrlen(char *str){
   size_t len;
   len = strlen(str);

   if(len < 0){
      perror("strlen");
      exit(-1);
   }
   return len;
}


int sfork(){
   int pid;
   if((pid = fork()) < 0){
      perror("Server fork");
      exit(-1);
   }
   return pid;
}


FILE* sfopen(char *path, char *mode){
   FILE *my_file = fopen(path, mode);
   if(my_file == NULL){
      perror("fopen");
      return NULL;
   }
   return my_file;
}


size_t sfread(void *ptr, size_t size, size_t bs, FILE *stream){
   size_t amount;
   int err_val = 0;

   smemset(ptr, '\0', MAX_BUFF);
   amount = (fread(ptr, size, bs, stream));
   if(amount != bs){
      if((err_val = feof(stream)) <= 0){
         perror("Read call");
         exit(-1);
      }else{
         return 0;
      }
   }

   return (long)amount;

}