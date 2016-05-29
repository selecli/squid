#ifndef LIBVERIFYTS_H_
#define LIBVERIFYTS_H_


#ifdef __cplusplus
extern "C" {
#endif



// 验证key
// lUserId 恒定为 0
// pEncryptData key值
// lDataLen key长度
// 认证成功：返回0
// 认证失败：返回其他值
int qmusic_verifyTstreamKey(  int lUserId, const char *pEncryptData, const int lDataLen );

#ifdef __cplusplus
} /* extern "C" */
#endif



#endif /*LIBVERIFYTS_H_*/
