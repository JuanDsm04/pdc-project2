//bruteforce_mpi.c
//uso mpirun -np N ./bruteforce_mpi <modo> <cifrado.bin> <palabra> [bits]
//modos seq, naive

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "des_utils.h"

#define CHECK_EVERY 1024 //cada cuantas llaves se revisa si otro proceso ya encontro la solucion

typedef struct {
  const char *ciph;
  int len;
  char *tmp;
  const char *search;
  long iters;
  MPI_Request req;
} Ctx;

//prueba una llave, devuelve 1 si es la solucion, 2 si otro proceso ya la encontro, 0 si sigue
static int paso(Ctx *c, long key){
  c->iters++;
  if((c->iters & (CHECK_EVERY-1)) == 0){
    int flag;
    MPI_Test(&c->req, &flag, MPI_STATUS_IGNORE);
    if(flag) return 2;
  }
  return tryKey(key, c->ciph, c->len, c->tmp, c->search) ? 1 : 0;
}

//cada proceso recorre un segmento contiguo del espacio
static int buscarNaive(Ctx *c, int id, int N, long upper, long *key){
  long range = upper / N;
  long lower = range * id;
  long top = (id == N-1) ? upper : range * (id+1);
  for(long i = lower; i < top; ++i){
    int r = paso(c, i);
    if(r == 1){ *key = i; return 1; }
    if(r == 2) return 0;
  }
  return 0;
}

int main(int argc, char *argv[]){
  MPI_Init(&argc, &argv);
  int N, id;
  MPI_Comm_size(MPI_COMM_WORLD, &N);
  MPI_Comm_rank(MPI_COMM_WORLD, &id);

  //programacion defensiva, todos los procesos validan igual y salen juntos
  int bits = 56;
  int err = 0;
  if(argc < 4 || argc > 5){
    err = 1;
  }else{
    if(argc == 5) bits = atoi(argv[4]);
    if(bits < 1 || bits > 56) err = 1;
    if(strcmp(argv[1], "seq") == 0 && N != 1) err = 2;
    else if(strcmp(argv[1], "naive") && strcmp(argv[1], "seq")) err = 1;
    if(strlen(argv[3]) == 0) err = 1;
  }
  if(err){
    if(id == 0){
      if(err == 2) fprintf(stderr, "el modo seq se ejecuta con -np 1\n");
      else fprintf(stderr, "uso mpirun -np N %s <seq|naive> <cifrado.bin> <palabra> [bits 1-56]\n", argv[0]);
    }
    MPI_Finalize();
    return 1;
  }

  const char *modo = argv[1];
  int len = 0;
  char *ciph = NULL;
  if(id == 0){
    ciph = leerArchivo(argv[2], &len);
    if(!ciph) len = -1;
  }
  MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if(len < 0){
    MPI_Finalize();
    return 1;
  }
  if(id != 0) ciph = malloc(len + 1);
  MPI_Bcast(ciph, len, MPI_CHAR, 0, MPI_COMM_WORLD);

  Ctx c;
  c.ciph = ciph;
  c.len = len;
  c.tmp = malloc(len + 1);
  c.search = argv[3];
  c.iters = 0;

  long upper = 1L << bits;
  long key = 0;
  long recv = 0;

  //los avisos de llave encontrada viajan en un comunicador propio
  MPI_Comm stopComm;
  MPI_Comm_dup(MPI_COMM_WORLD, &stopComm);
  MPI_Irecv(&recv, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, stopComm, &c.req);
  MPI_Barrier(MPI_COMM_WORLD);
  double t0 = MPI_Wtime();

  int hit = buscarNaive(&c, id, N, upper, &key);

  if(hit){
    for(int node = 0; node < N; node++){
      if(node != id) MPI_Send(&key, 1, MPI_LONG, node, 0, stopComm);
    }
  }

  //se comparte quien encontro la llave, si nadie la encontro todos terminan igual
  long mine = hit ? key + 1 : 0, best = 0;
  MPI_Allreduce(&mine, &best, 1, MPI_LONG, MPI_MAX, MPI_COMM_WORLD);
  if(c.req != MPI_REQUEST_NULL){
    if(hit || best == 0) MPI_Cancel(&c.req);
    MPI_Wait(&c.req, MPI_STATUS_IGNORE);
  }
  double dt = MPI_Wtime() - t0, tmax = 0;
  MPI_Reduce(&dt, &tmax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  long *all = id == 0 ? malloc(N * sizeof(long)) : NULL;
  MPI_Gather(&c.iters, 1, MPI_LONG, all, 1, MPI_LONG, 0, MPI_COMM_WORLD);

  if(id == 0){
    long total = 0;
    for(int i = 0; i < N; i++) total += all[i];
    printf("archivo = %s\n", argv[2]);
    printf("palabra clave = %s\n", argv[3]);
    printf("modo = %s, procesos = %d, bits = %d\n", modo, N, bits);
    if(best == 0){
      printf("llave no encontrada en el espacio de %d bits\n", bits);
    }else{
      char *plain = malloc(len + 1);
      memcpy(plain, ciph, len);
      plain[len] = 0;
      decrypt(best - 1, plain, len);
      printf("llave encontrada = %ld\n", best - 1);
      printf("texto descifrado = %s\n", plain);
      free(plain);
    }
    printf("tiempo (s) = %.6f\n", tmax);
    printf("iteraciones por proceso =");
    for(int i = 0; i < N; i++) printf(" %ld", all[i]);
    printf("\niteraciones totales = %ld\n", total);
    fflush(stdout);
    free(all);
  }

  MPI_Comm_free(&stopComm);
  free(c.tmp);
  free(ciph);
  MPI_Finalize();
  return 0;
}
