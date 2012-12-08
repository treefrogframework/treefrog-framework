#include <QString>
#include "md5.h"

/*
  The MD5 Message-Digest Algorithm
  This code is taken from: http://www.faqs.org/rfcs/rfc1321.html
 */
const int S11 = 7;
const int S12 = 12;
const int S13 = 17;
const int S14 = 22;
const int S21 = 5;
const int S22 = 9;
const int S23 = 14;
const int S24 = 20;
const int S31 = 4;
const int S32 = 11;
const int S33 = 16;
const int S34 = 23;
const int S41 = 6;
const int S42 = 10;
const int S43 = 15;
const int S44 = 21;

/* rotateLeft rotates x left n bits. */
inline quint32 rotateLeft(quint32 x, quint32 n) { return ((x << n) | (x >> (32 - n))); }


/* F, G, H and I are basic MD5 functions. */
inline quint32 F(quint32 x, quint32 y, quint32 z) { return (x & y) | ((~x) & z); }
inline quint32 G(quint32 x, quint32 y, quint32 z) { return ((x & z) | (y & (~z))); }
inline quint32 H(quint32 x, quint32 y, quint32 z) { return (x ^ y ^ z); }
inline quint32 I(quint32 x, quint32 y, quint32 z) { return (y ^ (x | ~z)); }


/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
   Rotation is separate from addition to prevent recomputation. */
inline void FF(quint32 &a, quint32 b, quint32 c, quint32 d, quint32 x, quint32 s, quint32 ac)
{
    a += F(b, c, d) + x + (quint32)(ac);
    a = rotateLeft(a, s) + b; 
}

inline void GG(quint32 &a, quint32 b, quint32 c, quint32 d, quint32 x, quint32 s, quint32 ac)
{
    a += G(b, c, d) + x + (quint32)(ac);
    a = rotateLeft(a, s) + b;
}

inline void HH(quint32 &a, quint32 b, quint32 c, quint32 d, quint32 x, quint32 s, quint32 ac)
{
    a += H(b, c, d) + x + (quint32)(ac); 
    a = rotateLeft(a, s) + b; 
}

inline void II(quint32 &a, quint32 b, quint32 c, quint32 d, quint32 x, quint32 s, quint32 ac)
{
    a += I(b, c, d) + x + (quint32)(ac);
    a = rotateLeft(a, s) + b;
}


static uchar PADDING[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* MD5 initialization. Begins an MD5 operation, writing a new context.
 */
void MD5Init(MD5_CTX *context)
{
    context->count[0] = context->count[1] = 0;
    /* Load magic initialization constants. */
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}


/* MD5 block update operation. Continues an MD5 message-digest
  operation, processing another message block, and updating the
  context.
 */
void MD5Update(MD5_CTX *context, const uchar *input, uint inputLen
)
{
    uint i, index, partLen;
    
    /* Compute number of bytes mod 64 */
    index = (uint)((context->count[0] >> 3) & 0x3F);
    
    /* Update number of bits */
    if ((context->count[0] += ((quint32)inputLen << 3)) < ((quint32)inputLen << 3)) {
        context->count[1]++;
    }
    
    context->count[1] += ((quint32)inputLen >> 29);    
    partLen = 64 - index;
    
    /* Transform as many times as possible. */
    if (inputLen >= partLen) {
        memcpy(&context->buffer[index], input, partLen);
        MD5Transform(context->state, context->buffer);
        
        for (i = partLen; i + 63 < inputLen; i += 64)
            MD5Transform(context->state, &input[i]);
        
        index = 0;
    } else {
        i = 0;    
    }
    
    /* Buffer remaining input */
    memcpy(&context->buffer[index], &input[i], inputLen - i);
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the
  the message digest and zeroizing the context.
 */
void MD5Final(uchar digest[16], MD5_CTX *context)
{
    uchar bits[8];
    uint index, padLen;
    
    /* Save number of bits */
    Encode(bits, context->count, 8);

    /* Pad out to 56 mod 64. */
    index = (uint)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD5Update(context, PADDING, padLen);
    
    /* Append length (before padding) */
    MD5Update(context, bits, 8);
    
    /* Store state in digest */
    Encode(digest, context->state, 16);
    
    /* Zeroize sensitive information.
     */
    memset(context, 0, sizeof (*context));
}


/* MD5 basic transformation. Transforms state based on block.
 */
void MD5Transform(quint32 state[4], const uchar block[64])
{
    quint32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    
    Decode(x, block, 64);
        
    /* Round 1 */
    FF(a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
    FF(d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
    FF(c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
    FF(b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
    FF(a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
    FF(d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
    FF(c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
    FF(b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
    FF(a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
    FF(d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */
    
    /* Round 2 */
    GG(a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
    GG(d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
    GG(d, a, b, c, x[10], S22,  0x2441453); /* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
    
    GG(b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
    GG(c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */
    
    /* Round 3 */
    HH(a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
    HH(d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH(a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
    HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
    HH(a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH(b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */
    
    /* Round 4 */
    II(a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
    II(d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II(b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II(b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II(c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II(a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */
    
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    
    /* Zeroize sensitive information. */
    memset(x, 0, sizeof(x));
}


/* Encodes input (quint32) into output (uchar). Assumes len is
  a multiple of 4. */
void Encode(uchar *output, quint32 *input, uint len)
{
    for (uint i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (uchar)(input[i] & 0xff);
        output[j+1] = (uchar)((input[i] >> 8) & 0xff);
        output[j+2] = (uchar)((input[i] >> 16) & 0xff);
        output[j+3] = (uchar)((input[i] >> 24) & 0xff);
    }
}


/* Decodes input (uchar) into output (quint32). Assumes len is
  a multiple of 4.
 */
void Decode(quint32 *output, const uchar *input, uint len)
{
    for (uint i = 0, j = 0; j < len; i++, j += 4)
        output[i] = ((quint32)input[j]) | (((quint32)input[j+1]) << 8) |
            (((quint32)input[j+2]) << 16) | (((quint32)input[j+3]) << 24);
}



/*
  Function: hmac_md5
  HMAC MD5 as listed in RFC 2104
  This code is taken from: http://www.faqs.org/rfcs/rfc2104.html
*/

#include <QCryptographicHash>

void hmac_md5(
    const uchar *text,       /* pointer to data stream */
    int          text_len,   /* length of data stream */
    const uchar *key,        /* pointer to authentication key */
    int          key_len,    /* length of authentication key */
    uchar *      digest)     /* caller digest to be filled in */
{
    uchar tk[16];
    /* if key is longer than 64 bytes reset it to key=MD5(key) */
    if (key_len > 64) {
#if 0
        MD5_CTX tctx;
        MD5Init(&tctx);
        MD5Update(&tctx, key, key_len);
        MD5Final(tk, &tctx);
        
        key = tk;
        key_len = 16;
#else
        QByteArray data((const char*)key, key_len);
        QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
        memcpy(tk, hash.data(), 16);
        key = tk;
        key_len = 16;
#endif
    }
    
    uchar k_ipad[65];  /* inner padding - key XORd with ipad */
    uchar k_opad[65];  /* outer padding - key XORd with opad */
    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (int i = 0; i < 64; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

#if 0
    /* perform inner MD5 */
    MD5_CTX context;
    MD5Init(&context);                   /* init context for 1st pass */
    MD5Update(&context, k_ipad, 64);     /* start with inner pad */
    MD5Update(&context, text, text_len); /* then text of datagram */
    MD5Final(digest, &context);          /* finish up 1st pass */

    /* perform outer MD5 */
    MD5Init(&context);                   /* init context for 2nd pass */
    MD5Update(&context, k_opad, 64);     /* start with outer pad */
    MD5Update(&context, digest, 16);     /* then results of 1st hash */
    MD5Final(digest, &context);          /* finish up 2nd pass */

#else
    QByteArray data;
    data.append((const char*)k_ipad, 64);
    data.append((const char*)text, text_len);
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    data.clear();
    data.append((const char*)k_opad, 64);
    data.append(hash.data(), 16);
    hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    memcpy(digest, hash.data(), 16);
#endif
}
