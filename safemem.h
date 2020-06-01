#ifndef SAFEMEM_H
#define SAFEMEM_H
void *smemcpy(void *dest, const void *src, size_t n);
void *smalloc(size_t size);
void *smemset(void *s, int c, size_t n);
void *srealloc(void *ptr, size_t size);
void * sCalloc(size_t nmemb, size_t size);
void sstrcpy(char *dest, const char *src);
size_t sstrlen(char *str);
int sfork();
FILE* sfopen(char *path, char *mode);
size_t sfread(void *ptr, size_t size, size_t bs, FILE *stream);
#endif