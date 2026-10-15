//cifrar.c
//uso ./cifrar cifrar <llave> <entrada.txt> <salida.bin>
//    ./cifrar descifrar <llave> <entrada.bin>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "des_utils.h"

int main(int argc, char *argv[]){
  unsigned long key;
  int len;

  if(argc < 4){
    fprintf(stderr, "uso %s cifrar <llave> <entrada.txt> <salida.bin>\n", argv[0]);
    fprintf(stderr, "   %s descifrar <llave> <entrada.bin>\n", argv[0]);
    return 1;
  }
  if(!parsearLlave(argv[2], &key)){
    fprintf(stderr, "llave invalida, debe ser un entero entre 0 y 2^56-1\n");
    return 1;
  }
  char *data = leerArchivo(argv[3], &len);
  if(!data) return 1;

  if(strcmp(argv[1], "cifrar") == 0 && argc == 5){
    encrypt(key, data, len);
    if(escribirArchivo(argv[4], data, len)){
      free(data);
      return 1;
    }
    printf("llave %lu, %d bytes cifrados en %s\n", key, len, argv[4]);
  }else if(strcmp(argv[1], "descifrar") == 0 && argc == 4){
    decrypt(key, data, len);
    printf("llave %lu, texto descifrado\n%s\n", key, data);
  }else{
    fprintf(stderr, "modo o argumentos invalidos\n");
    free(data);
    return 1;
  }
  free(data);
  return 0;
}
