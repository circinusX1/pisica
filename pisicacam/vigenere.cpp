
#include "vigenere.h"

std::string AVAILABLE_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";

int index(char c) {
    for(size_t ii = 0; ii < AVAILABLE_CHARS.size(); ii++) {
        if(AVAILABLE_CHARS[ii] == c) {
            // std::cout << ii << " " << c << std::endl;
            return ii;
        }
    }
    return -1;
}


std::string extend_key(std::string& msg, std::string& key) {
    //generating new key
    int msgLen = msg.size();
    std::string newKey(msgLen, 'x');
    int keyLen = key.size(), i, j;
    for(i = 0, j = 0; i < msgLen; ++i, ++j){
        if(j == keyLen)
            j = 0;

        newKey[i] = key[j];
    }
    newKey[i] = '\0';
    return newKey;
}


std::string encrypt_vigenere(std::string& msg, std::string& key) {
    int msgLen = msg.size(), keyLen = key.size(), i, j;
    (void)(j);
    (void)(keyLen);
    std::string encryptedMsg(msgLen, 'x');
    // char newKey[msgLen], encryptedMsg[msgLen], decryptedMsg[msgLen];

    std::string newKey = extend_key(msg, key);

    //encryption
    for(i = 0; i < msgLen; ++i) {
        // std::cout << msg[i] << " " << isalnum(msg[i]) << std::endl;
        if(isalnum(msg[i]) or msg[i] == ' ') {
            encryptedMsg[i] = AVAILABLE_CHARS[((index(msg[i]) + index(newKey[i])) % AVAILABLE_CHARS.size())];
        } else {
            encryptedMsg[i] = msg[i];
        }
    }

    encryptedMsg[i] = '\0';
    return encryptedMsg;
}

std::string decrypt_vigenere(std::string& encryptedMsg, std::string& newKey) {
    // decryption
    int msgLen = encryptedMsg.size();
    std::string decryptedMsg(msgLen, 'x');
    int i;
    for(i = 0; i < msgLen; ++i) {
        if(isalnum(encryptedMsg[i]) or encryptedMsg[i] == ' ') {
            decryptedMsg[i] = AVAILABLE_CHARS[(((index(encryptedMsg[i]) - index(newKey[i])) + AVAILABLE_CHARS.size()) % AVAILABLE_CHARS.size())];
        } else {
            decryptedMsg[i] = encryptedMsg[i];
        }
    }
    decryptedMsg[i] = '\0';
    return decryptedMsg;
}


inline std::string encrypt(const std::string& msg, std::string& key) {
    std::string b64_str = base64_encode(msg);
    std::string vigenere_msg = encrypt_vigenere(b64_str, key);
    // std::cout << vigenere_msg << std::endl;
    return vigenere_msg;
}


inline std::string decrypt( std::string& encrypted_msg,  std::string& key) {
    std::string newKey = extend_key(encrypted_msg, key);
    std::string b64_encoded_str = decrypt_vigenere(encrypted_msg, newKey);
    std::string b64_decode_str = base64_decode(b64_encoded_str);
    return b64_decode_str;
}

