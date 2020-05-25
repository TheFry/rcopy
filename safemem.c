/* Wrapper that checks return values of standard memory libs **/

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include "safemem.h"

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
   void *new = NULL;
   new = realloc(ptr, size);

   if(new == NULL){
      perror("realloc");
      exit(-1);
   }

   return new;
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