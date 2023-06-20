#ifndef BASE64_H
#define BASE64_H

#include <string>

namespace Base64{
    std::string base64_encode(const char* bytes_to_encode, size_t len);
    std::string base64_decode(const std::string& encoded_string);
}

#endif
