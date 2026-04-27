#include "Serializer.h"
#include <cstdint>

std::vector<char> Serializer::serialize(const std::map<std::string, std::string>& data){
    std::vector<char> buffer;
    // 1. Número de llaves (2 bytes)
    uint16_t keyCount = data.size();
    buffer.push_back(keyCount & 0xFF);
    buffer.push_back((keyCount >> 8) & 0xFF);

    for (auto const& [key, val] : data)
    {
        // 2. Longitud Llave (1 byte, max 255 chars)
        buffer.push_back((uint8_t)key.length());
        // 3. Texto Llave
        buffer.insert(buffer.end(), key.begin(), key.end());
        // 4. Longitud Valor (1 byte)
        buffer.push_back((uint8_t)val.length());
        // 5. Texto Valor
        buffer.insert(buffer.end(), val.begin(), val.end());
    }
    return buffer;
}