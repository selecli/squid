#ifndef SHAMC_SHA256_H
#define SHAMC_SHA256_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <stdint.h>     //  Added by RKW, needed for types uint8_t, uint32_t; requires C99 compiler


typedef struct {
	uint8_t     hash[32];       // Changed by RKW, unsigned char becomes uint8_t
	uint32_t    buffer[16];     // Changed by RKW, unsigned long becomes uint32_t
	uint32_t    state[8];       // Changed by RKW, unsinged long becomes uint32_t
	uint8_t     length[8];      // Changed by RKW, unsigned char becomes uint8_t
} sha256; 

typedef struct _hmac_sha256 {
	uint8_t     digest[32];     // Changed by RKW, unsigned char becomes uint_8
	uint8_t     key[64];        // Changed by RKW, unsigned char becomes uint_8
	sha256      sha;
} hmac_sha256;

extern void hmac_sha2(unsigned char *key, int lk, unsigned char *msg, int lm, unsigned char *out);


#endif	// SHAMC_SHA256_H
