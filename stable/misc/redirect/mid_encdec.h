#ifndef _INCLUDED_MUSICENCRYPT_H_
#define _INCLUDED_MUSICENCRYPT_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief decrypt song id
 * @param {INPUT} chId: encrypted string buffer
 * @return decrypted digital string
 */
unsigned long MusicDecryptSongId(char const * chId);

/**
 * @brief decrypt singer id
 * @param {INPUT} chId: encrypted string buffer
 * @return decrypted digital string
 */
unsigned long MusicDecryptSingerId(char const * chId);

/**
 * @brief decrypt album id
 * @param {INPUT} chId: encrypted string buffer
 * @return decrypted digital string
 */
unsigned long MusicDecryptAlbumId(char const * chId);

/**
 * @brief  encrypt song id
 *
 * @param  {INPUT} uId: digital numer to be encrypted
 * @param  {OUTPUT} chOutBuf: encrypted string buffer. Recommand to feed in char chOutBuf[16] and use sizeof(chOutBuf) to get output size.
 * @return none
 */
void MusicEncryptSongId(unsigned long uId, char* chOutBuf);

/**
 * @brief  encrypt singer id
 *
 * @param  {INPUT} uId: digital numer to be encrypted
 * @param  {OUTPUT} chOutBuf: encrypted string buffer. Recommand to feed in char chOutBuf[16] and use sizeof(chOutBuf) to get output size.
 * @return none
 */
void MusicEncryptSingerId(unsigned long uId, char* chOutBuf);

/**
 * @brief  encrypt album id
 *
 * @param  {INPUT} uId: digital numer to be encrypted
 * @param  {OUTPUT} chOutBuf: encrypted string buffer. Recommand to feed in char chOutBuf[16] and use sizeof(chOutBuf) to get output size.
 * @return none
 */
void MusicEncryptAlbumId(unsigned long uId, char* chOutBuf);

/* for cpp, please use a wrapper like this:
string MusicEncryptSongId(unsigned long id)
{
  char outBuf[16] = {0};
  MusicEncryptSongId(id, outBuf);
  return string(outBuf, sizeof(outBuf));
}
*/

#ifdef __cplusplus
}
#endif

#endif
