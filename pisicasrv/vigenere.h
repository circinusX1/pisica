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

#endif //
