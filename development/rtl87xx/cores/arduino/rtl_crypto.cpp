
#include "rtl_crypto.h"
#include "Arduino.h"

void renderHexdata(char* out, unsigned int outsize,  uint8_t* data, unsigned int datasize ){
  const char table[]="0123456789abcdef";
  for (int i=0; i < datasize; ++i){
    if((i << 1) + 1 >= outsize - 1 )
      break;
    out[i << 1] = table[data[i] >> 4];
    out[(i << 1) + 1] = table[data[i] & 0x0f];
  }
  out[outsize-1] = '\0';
}

MD5Hash::MD5Hash (const MD5Hash &other){
  memcpy(hash,other.hash,16);
}

MD5Hash& MD5Hash::operator= (const MD5Hash &other){
   memcpy(hash,other.hash,16);
   return *this;
}

String MD5Hash::toString(){
  char ret[33];
  
  renderHexdata(ret,sizeof(ret),hash,sizeof(hash));
  return String(ret);
}



int InitCryptoEngine(){
  return rtl_cryptoEngine_init();
}

MD5Hash computeMD5(const char* data, unsigned int size){
  MD5Hash ret;
  rtl_crypto_md5(reinterpret_cast<const uint8_t*>(data), size, ret.data());

  return ret;
}
