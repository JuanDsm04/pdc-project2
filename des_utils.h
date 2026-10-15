//des_utils.h
//rutinas compartidas de cifrado DES y manejo de archivos

#ifndef DES_UTILS_H
#define DES_UTILS_H

//cifra in-place len bytes (multiplo de 8) con la llave de 56 bits
void encrypt(unsigned long key, char *data, int len);

//descifra in-place len bytes (multiplo de 8) con la llave de 56 bits
void decrypt(unsigned long key, char *data, int len);

//descifra una copia de ciph en tmp (de al menos len+1 bytes) y busca la palabra clave
int tryKey(unsigned long key, const char *ciph, int len, char *tmp, const char *search);

//lee un archivo completo en un buffer nuevo con padding de ceros a multiplo de 8
//devuelve NULL si falla, out_len recibe la longitud con padding
char *leerArchivo(const char *path, int *out_len);

//escribe len bytes en un archivo, devuelve 0 si tuvo exito
int escribirArchivo(const char *path, const char *data, int len);

//convierte una cadena decimal a llave, devuelve 0 si es invalida o excede 56 bits
int parsearLlave(const char *s, unsigned long *key);

#endif
