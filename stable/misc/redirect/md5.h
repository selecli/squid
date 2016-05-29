#ifndef MD5_H
#define MD5_H

/* MD5 context. */
typedef struct {
  unsigned int state[4];                                   /* state (ABCD) */
  unsigned int count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char [16], MD5_CTX *);


void getMd5Digest(char* buffer, int length, unsigned char* digest);
void getMd5Digest32(char* buffer, int length, unsigned char* digest);

#endif	//MD5_H
