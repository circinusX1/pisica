#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <ctype.h>

#ifndef VINEGERE_H
#define  VINEGERE_H

using namespace std;

extern std::string AVAILABLE_CHARS;// = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";

int index(char c);
std::string extend_key(std::string& msg, std::string& key);
std::string encrypt_vigenere(std::string& msg, std::string& key);
std::string decrypt_vigenere(std::string& encryptedMsg, std::string& newKey) ;


#include <string>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;


static std::string base64_encode(const std::string &in) {

    std::string out;

    int val=0, valb=-6;
    for (size_t jj = 0; jj < in.size(); jj++) {
        char c = in[jj];
        val = (val<<8) + c;
        valb += 8;
        while (valb>=0) {
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
            valb-=6;
        }
    }
    if (valb>-6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
    while (out.size()%4) out.push_back('=');
    return out;
}

static std::string base64_decode(const std::string &in) {

    std::string out;

    std::vector<int> T(256,-1);
    for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    int val=0, valb=-8;
    for (size_t jj = 0; jj < in.size(); jj++) {
        char c = in[jj];
        if (T[c] == -1) break;
        val = (val<<6) + T[c];
        valb += 6;
        if (valb>=0) {
            out.push_back(char((val>>valb)&0xFF));
            valb-=8;
        }
    }
    return out;
}

inline std::string xencrypt(const std::string& msg, std::string& key) {
    std::string b64_str = base64_encode(msg);
    std::string vigenere_msg = encrypt_vigenere(b64_str, key);
    // std::cout << vigenere_msg << std::endl;
    return vigenere_msg;
}


inline std::string xdecrypt( std::string& encrypted_msg,  std::string& key) {
    std::string newKey = extend_key(encrypted_msg, key);
    std::string b64_encoded_str = decrypt_vigenere(encrypted_msg, newKey);
    std::string b64_decode_str = base64_decode(b64_encoded_str);
    return b64_decode_str;
}



#endif //
