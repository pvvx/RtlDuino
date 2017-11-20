#ifndef _RTL_CRYPTO_H
#define _RTL_CRYPTO_H

#include "WString.h"

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus
#include "hal_crypto.h"
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

class MD5Hash {
public:
    MD5Hash (){}
    MD5Hash (const MD5Hash &other);
    MD5Hash& operator= (const MD5Hash &other);
    uint8_t* data(){ return hash; }
    unsigned int size(){ return 16; }
    String toString();

private:
  uint8_t hash[16];
};

int InitCryptoEngine();

MD5Hash computeMD5(const char* data, unsigned int size);
void renderHexdata(char* out, unsigned int outsize,  uint8_t* data, unsigned int datasize );






#endif //_RTL_CRYPTO_H
