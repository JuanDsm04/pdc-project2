CC = mpicc
CFLAGS = -O2 -Wall -Wno-deprecated-declarations
LIBS = -lcrypto

all: cifrar

cifrar: cifrar.c des_utils.c des_utils.h
	$(CC) $(CFLAGS) -o $@ cifrar.c des_utils.c $(LIBS)

clean:
	rm -f cifrar bruteforce_mpi
