# Proyecto 2 - Computación Paralela y Distribuida

Búsqueda por fuerza bruta de una llave DES usando MPI.

Autores: Juan Diego Solís Martínez (23720), Victor Manuel Pérez Chávez (23731), Luis Fernando Palacios López (23933).

## Requisitos

- `mpicc` (OpenMPI o MPICH)
- `g++` (para la versión secuencial de la Parte A)
- Librería OpenSSL (`libssl-dev` / `openssl-devel`), por el uso de `openssl/des.h`

En Debian/Ubuntu (incluye WSL):

```
sudo apt install openmpi-bin libopenmpi-dev libssl-dev g++
```

## Compilación

Con `make` se compilan `cifrar` y `bruteforce_mpi` (Parte B):

```
make
```

`bruteforce.c` y `des-secuencial.cpp` (Parte A) no están en el Makefile y se compilan aparte, como se indica abajo.

## Parte A

### des-secuencial.cpp

Búsqueda secuencial de DES para llaves de 16, 18, 20 y 22 bits, usada para generar la Tabla 1 del informe.

```
g++ -O2 des-secuencial.cpp -o des-secuencial
./des-secuencial
```

### bruteforce.c

Busca por fuerza bruta, con MPI, la llave que descifra el arreglo `cipher[]` incluido en el código, buscando la palabra clave `" the "`. Usa un espacio de 2^56 llaves con una llave efectiva de 28 bits (ver el informe, sección "Funcionamiento del programa bruteforce.c").

```
mpicc -O2 bruteforce.c -o bruteforce -lcrypto -Wno-deprecated-declarations
mpirun -np 4 ./bruteforce
```

Debe encontrar la llave `105734` y mostrar el texto `Save the planet `.

## Parte B

### cifrar

Cifra o descifra un archivo de texto con una llave de hasta 56 bits (2^56 - 1).

```
./cifrar cifrar <llave> <entrada.txt> <salida.bin>
./cifrar descifrar <llave> <entrada.bin>
```

Ejemplo, usando `prueba.txt`:

```
./cifrar cifrar 42 prueba.txt prueba.bin
./cifrar descifrar 42 prueba.bin
```

### bruteforce_mpi

Busca con MPI la llave que descifra un archivo cifrado y produce un texto que contiene la palabra clave dada. Soporta cinco modos de reparto del espacio de búsqueda.

```
mpirun -np N ./bruteforce_mpi <modo> <cifrado.bin> <palabra> [bits] [bloque]
```

- `modo`: `seq` (secuencial, requiere `-np 1`), `naive` (segmentos contiguos), `stride` (entrelazado), `blocks` (bloques cíclicos de tamaño `bloque`), `dynamic` (contador compartido con MPI_Fetch_and_op).
- `cifrado.bin`: archivo generado con `cifrar`.
- `palabra`: palabra o frase que debe aparecer en el texto descifrado.
- `bits` (opcional, 1-56, por defecto 56): tamaño del espacio de búsqueda, 2^bits.
- `bloque` (opcional, por defecto 65536): tamaño de bloque para los modos `blocks` y `dynamic`.

Ejemplo:

```
./cifrar cifrar 42 prueba.txt prueba.bin
mpirun -np 4 ./bruteforce_mpi naive prueba.bin "prueba" 26
mpirun -np 4 ./bruteforce_mpi stride prueba.bin "prueba" 26
mpirun -np 4 ./bruteforce_mpi blocks prueba.bin "prueba" 26 65536
mpirun -np 4 ./bruteforce_mpi dynamic prueba.bin "prueba" 26 65536
mpirun -np 1 ./bruteforce_mpi seq prueba.bin "prueba" 26
```

Cada corrida imprime el archivo, la palabra clave, el modo, el número de procesos y bits usados, la llave encontrada (o el aviso de que no se encontró) junto con el texto descifrado, el tiempo en segundos y las iteraciones por proceso y totales.

## Limpieza

```
make clean
```

## Informe y repositorio

El informe completo (`pdc-project2.docx`) documenta la investigación de DES, el funcionamiento de cada programa, las pruebas realizadas y las conclusiones. Repositorio: https://github.com/JuanDsm04/pdc-project2.git
