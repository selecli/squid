#ifndef MYSPACE_AUTH_MUSIC_H
#define MYSPACE_AUTH_MUSIC_H

int getAuthenticMyspaceMusicRequestVersion(char* salt);
char* getTokenForDC(char* buffer,int bufLength);
int authenticMyspaceMusicRequest(char* userRequestURL, int strLength);

#endif
