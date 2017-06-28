#ifndef ALGORITHM_UTILS_H
#define ALGORITHM_UTILS_H
#include <string>
#include <memory>
#include <functional>
#include <stdio.h>

template<typename T>
using super_ptr = std::unique_ptr<T, std::function<void (T*)>>;

typedef struct
{
    unsigned int count[2];
    unsigned int state[4];
    unsigned char buffer[64];
}MD5_CTX;


std::string get_file_md5 (const char* path);

std::string convert_to_hex (const std::string&);

void MD5Transform(unsigned int state[4],unsigned char block[64]);

void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len);

void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len);

void MD5Final(MD5_CTX *context,unsigned char digest[16]);

void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen);

void MD5Init(MD5_CTX *context);

void MD5Transform(unsigned int state[4],unsigned char block[64]);

#endif // ALGORITHM_UTILS_H

