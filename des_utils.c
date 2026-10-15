//des_utils.c

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/des.h>
#include "des_utils.h"

//nota de adaptacion: equivalente a ecb_crypt() de rpc/des_crypt.h
static void ecb_crypt(char *key, char *data, int len, int mode){
  DES_key_schedule ks;
  DES_set_key_unchecked((DES_cblock *)key, &ks);
  for(int off=0; off<len; off+=8){
    DES_ecb_encrypt((DES_cblock *)(data+off), (DES_cblock *)(data+off), &ks, mode);
  }
}

//reparte los 56 bits de la llave en 8 bytes (7 bits utiles por byte) y fija la paridad
//nota: bruteforce.c usa 4 bytes (28 bits), aqui se usan los 8 para cubrir 2^56 llaves
static unsigned long armarLlave(unsigned long key){
  unsigned long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFEUL << i*8));
  }
  DES_set_odd_parity((DES_cblock *)&k);
  return k;
}

void decrypt(unsigned long key, char *data, int len){
  unsigned long k = armarLlave(key);
  ecb_crypt((char *)&k, data, len, DES_DECRYPT);
}

void encrypt(unsigned long key, char *data, int len){
  unsigned long k = armarLlave(key);
  ecb_crypt((char *)&k, data, len, DES_ENCRYPT);
}

int tryKey(unsigned long key, const char *ciph, int len, char *tmp, const char *search){
  memcpy(tmp, ciph, len);
  tmp[len] = 0;
  decrypt(key, tmp, len);
  return strstr(tmp, search) != NULL;
}

char *leerArchivo(const char *path, int *out_len){
  FILE *f = fopen(path, "rb");
  if(!f){
    fprintf(stderr, "no se pudo abrir %s (%s)\n", path, strerror(errno));
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if(size <= 0 || size > (1L<<26)){
    fprintf(stderr, "tamanio invalido en %s\n", path);
    fclose(f);
    return NULL;
  }
  int padded = (int)((size + 7) / 8 * 8);
  char *buf = calloc(padded + 1, 1);
  if(!buf || fread(buf, 1, size, f) != (size_t)size){
    fprintf(stderr, "no se pudo leer %s\n", path);
    free(buf);
    fclose(f);
    return NULL;
  }
  fclose(f);
  *out_len = padded;
  return buf;
}

int escribirArchivo(const char *path, const char *data, int len){
  FILE *f = fopen(path, "wb");
  if(!f){
    fprintf(stderr, "no se pudo crear %s (%s)\n", path, strerror(errno));
    return 1;
  }
  int ok = fwrite(data, 1, len, f) == (size_t)len;
  fclose(f);
  return ok ? 0 : 1;
}

int parsearLlave(const char *s, unsigned long *key){
  char *end;
  errno = 0;
  if(*s == '-' || *s == 0) return 0;
  unsigned long v = strtoul(s, &end, 10);
  if(errno || *end != 0 || v >= (1UL<<56)) return 0;
  *key = v;
  return 1;
}
