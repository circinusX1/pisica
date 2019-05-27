#include "b64.h"
#include "vigenere.h"

inline std::string enkr(const std::string& msg, std::string& key) {
	std::string b64_str = base64_encode(msg);
    std::string vigenere_msg = enkr(b64_str, key);
	// std::cout << vigenere_msg << std::endl;
	return vigenere_msg;
}


inline std::string dekr( std::string& encrypted_msg,  std::string& key) {
	std::string newKey = extend_key(encrypted_msg, key);
    std::string b64_encoded_str = dekr(encrypted_msg, newKey);
	std::string b64_decode_str = base64_decode(b64_encoded_str);
	return b64_decode_str;
}
