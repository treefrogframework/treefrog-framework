#ifndef MD5_H
#define MD5_H

#include <qglobal.h>


/* MD5 context. */
struct MD5_CTX {
    quint32 state[4];   /* state (ABCD) */
    quint32 count[2];   /* number of bits, modulo 2^64 (lsb first) */
    uchar buffer[64];   /* input buffer */
};


// The MD5 Message-Digest Algorithm
void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, const uchar *input, uint inputLen);
void MD5Final(uchar digest[16], MD5_CTX *context); 
void MD5Transform(quint32 state[4], const uchar block[64]);
void Encode(uchar *output, quint32 *input, uint len);
void Decode(quint32 *output, const uchar *input, uint len);


// HMAC MD5 function
void hmac_md5(const uchar *text, int text_len, const uchar *key, int key_len, uchar *digest);

#endif // MD5_H
