//bruteforce.c
//nota: el key usado es bastante pequenio, cuando sea random speedup variara

//nota de adaptacion: rpc/des_crypt.h no existe en Linux moderno, se reemplaza por openssl/des.h

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

//nota de adaptacion: equivalente a ecb_crypt() de rpc/des_crypt.h
void ecb_crypt(char *key, char *data, int len, int mode){
  DES_key_schedule ks;
  DES_set_key_unchecked((DES_cblock *)key, &ks);
  for(int off=0; off<len; off+=8){
    DES_ecb_encrypt((DES_cblock *)(data+off), (DES_cblock *)(data+off), &ks, mode);
  }
}

//nota de arreglo: corrimiento de 0xFE a mas de 31 bits es UB en int; se limita a 4 iteraciones (64 bits, literal long)
void decrypt(long key, char *ciph, int len){
  //set parity of key and do decrypt
  long k = 0;
  for(int i=0; i<4; ++i){
    key <<= 1;
    k += (key & (0xFEL << i*8));
  }
  DES_set_odd_parity((DES_cblock *)&k);  //equivalente a des_setparity
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_DECRYPT);
}

void encrypt(long key, char *ciph, int len){
  //set parity of key and do decrypt
  long k = 0;
  for(int i=0; i<4; ++i){
    key <<= 1;
    k += (key & (0xFEL << i*8));
  }
  DES_set_odd_parity((DES_cblock *)&k);  //equivalente a des_setparity
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_ENCRYPT);
}

char search[] = " the ";
int tryKey(long key, char *ciph, int len){
  char temp[len+1];
  memcpy(temp, ciph, len);
  temp[len]=0;
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

unsigned char cipher[] = {108, 245, 65, 63, 125, 200, 150, 66, 17, 170, 207, 170, 34, 31, 70, 215, 0};
int main(int argc, char *argv[]){ //char **argv
  int N, id;
  long upper = (1L <<56); //upper bound DES keys 2^56
  long mylower, myupper;
  MPI_Status st;
  MPI_Request req;
  int flag;
  int ciphlen = strlen((char *)cipher);
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Init(NULL, NULL);
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  //nota de arreglo: long en vez de int; con upper = 2^56 un int se desborda
  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id+1) -1;
  if(id == N-1){
    //compensar residuo
    myupper = upper;
  }

  long found = 0;
  long check_every = 1L<<16; //cada cuantas llaves se revisa si otro proceso ya encontro la solucion

  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

  for(long i = mylower; i<myupper && (found==0); ++i){
    //nota de arreglo: sin este chequeo periodico, un proceso recorre todo su rango aunque otro ya haya terminado
    if((i - mylower) % check_every == 0){
      MPI_Test(&req, &flag, &st);
      if(flag) break;
    }
    if(tryKey(i, (char *)cipher, ciphlen)){
      found = i;
      for(int node=0; node<N; node++){
        MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
      }
      break;
    }
  }

  //nota de arreglo: todos los procesos atienden su Irecv para no dejar nada pendiente antes de Finalize
  MPI_Wait(&req, &st);

  if(id==0){
    decrypt(found, (char *)cipher, ciphlen);
    printf("%li %s\n", found, cipher);
    fflush(stdout);
  }

  MPI_Finalize();
}
